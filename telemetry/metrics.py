#!/usr/bin/env python3
"""
FastAPI-based telemetry service for low-latency trading system.
Reads telemetry data and exposes it as real-time API endpoints.
"""

import asyncio
import csv
import json
import os
import time
from datetime import datetime, timedelta
from pathlib import Path
from typing import Dict, List, Optional, Tuple, Any

import uvicorn
from fastapi import FastAPI, HTTPException
from fastapi.responses import PlainTextResponse
from prometheus_client import Counter, Gauge, Histogram, generate_latest
from pydantic import BaseModel

# FastAPI app
app = FastAPI(
    title="Low-Latency Trading Telemetry",
    description="Real-time metrics API for trading system",
    version="1.0.0"
)

# Prometheus metrics
orders_produced_total = Counter(
    'orders_produced_total',
    'Total number of orders produced',
    ['producer_id']
)

orders_consumed_total = Counter(
    'orders_consumed_total', 
    'Total number of orders consumed',
    ['consumer_id']
)

batches_sent_total = Counter(
    'batches_sent_total',
    'Total number of batches sent'
)

batch_latency_seconds = Histogram(
    'batch_latency_seconds',
    'Batch processing latency in seconds',
    buckets=[0.001, 0.005, 0.01, 0.05, 0.1, 0.5, 1.0, 2.0, 5.0]
)

throughput_orders_per_second = Gauge(
    'throughput_orders_per_second',
    'Current throughput in orders per second'
)

buffer_utilization_percent = Gauge(
    'buffer_utilization_percent',
    'Ring buffer utilization percentage'
)

network_dropped_packets_total = Counter(
    'network_dropped_packets_total',
    'Total packets dropped by network simulation',
    ['network_type']
)

network_retransmissions_total = Counter(
    'network_retransmissions_total',
    'Total network retransmissions',
    ['network_type']
)

# Data models
class BatchStats(BaseModel):
    timestamp: datetime
    orders_produced: int
    orders_consumed: int
    batches_sent: int
    throughput_ops_per_sec: float
    avg_latency_us: float
    buffer_size: int
    buffer_capacity: int
    network_type: str

class SystemStats(BaseModel):
    total_orders_produced: int
    total_orders_consumed: int
    total_batches_sent: int
    current_throughput: float
    avg_batch_latency_us: float
    buffer_utilization_percent: float
    network_stats: Dict[str, Any]
    last_update: datetime

# Global state
latest_stats: Optional[SystemStats] = None
telemetry_file_path = Path("telemetry.log")
last_file_position = 0
is_running = False

class TelemetryReader:
    """Reads and parses telemetry data from the trading system."""
    
    def __init__(self, file_path: Path):
        self.file_path = file_path
        self.last_position = 0
        self.last_stats = None
        
    def parse_csv_line(self, line: str) -> Optional[Dict]:
        """Parse a CSV line from telemetry log."""
        try:
            # Expected format: timestamp_ns,orders_produced,orders_consumed,orders_dropped,buffer_size,buffer_capacity,throughput_ops_per_sec,avg_latency_ns,p95_latency_ns,p99_latency_ns,network_errors,batch_count
            reader = csv.reader([line])
            row = next(reader)
            
            if len(row) < 12:
                return None
                
            return {
                'timestamp_ns': int(row[0]),
                'orders_produced': int(row[1]),
                'orders_consumed': int(row[2]),
                'orders_dropped': int(row[3]),
                'buffer_size': int(row[4]),
                'buffer_capacity': int(row[5]),
                'throughput_ops_per_sec': float(row[6]),
                'avg_latency_ns': float(row[7]),
                'p95_latency_ns': float(row[8]),
                'p99_latency_ns': float(row[9]),
                'network_errors': int(row[10]),
                'batch_count': int(row[11])
            }
        except (ValueError, IndexError):
            return None
    
    def read_new_data(self) -> List[Dict]:
        """Read new data from telemetry file."""
        if not self.file_path.exists():
            return []
            
        new_data = []
        try:
            with open(self.file_path, 'r') as f:
                f.seek(self.last_position)
                for line in f:
                    parsed = self.parse_csv_line(line.strip())
                    if parsed:
                        new_data.append(parsed)
                self.last_position = f.tell()
        except Exception as e:
            print(f"Error reading telemetry file: {e}")
            
        return new_data

class MetricsCollector:
    """Collects and updates Prometheus metrics."""
    
    def __init__(self):
        self.reader = TelemetryReader(telemetry_file_path)
        self.last_update = time.time()
        self.last_stats = None
        
    def update_metrics(self, data: List[Dict]):
        """Update Prometheus metrics with new data."""
        if not data:
            return
            
        latest = data[-1]
        
        # Update counters
        orders_produced_total.labels(producer_id="all").inc(
            latest['orders_produced'] - (self.last_stats['orders_produced'] if self.last_stats else 0)
        )
        orders_consumed_total.labels(consumer_id="all").inc(
            latest['orders_consumed'] - (self.last_stats['orders_consumed'] if self.last_stats else 0)
        )
        batches_sent_total.inc(
            latest['batch_count'] - (self.last_stats['batch_count'] if self.last_stats else 0)
        )
        
        # Update gauges
        throughput_orders_per_second.set(latest['throughput_ops_per_sec'])
        
        buffer_utilization = (latest['buffer_size'] / latest['buffer_capacity']) * 100
        buffer_utilization_percent.set(buffer_utilization)
        
        # Update histogram
        if latest['avg_latency_ns'] > 0:
            batch_latency_seconds.observe(latest['avg_latency_ns'] / 1_000_000_000)  # Convert ns to seconds
        
        self.last_stats = latest
        self.last_update = time.time()
    
    def update_network_metrics(self, network_stats: Dict):
        """Update network-specific metrics."""
        if 'tcp' in network_stats:
            network_dropped_packets_total.labels(network_type='tcp').inc(
                network_stats['tcp'].get('dropped_packets', 0)
            )
            network_retransmissions_total.labels(network_type='tcp').inc(
                network_stats['tcp'].get('retransmissions', 0)
            )
        elif 'udp' in network_stats:
            network_dropped_packets_total.labels(network_type='udp').inc(
                network_stats['udp'].get('packets_dropped', 0)
            )
        # SHM has no dropped packets or retransmissions

# Global collector
collector = MetricsCollector()

async def background_metrics_collector():
    """Background task to continuously collect metrics."""
    global latest_stats, is_running
    
    is_running = True
    print("Starting background metrics collector...")
    
    while is_running:
        try:
            # Read new telemetry data
            new_data = collector.reader.read_new_data()
            
            if new_data:
                # Update Prometheus metrics
                collector.update_metrics(new_data)
                
                # Update latest stats
                latest = new_data[-1]
                latest_stats = SystemStats(
                    total_orders_produced=latest['orders_produced'],
                    total_orders_consumed=latest['orders_consumed'],
                    total_batches_sent=latest['batch_count'],
                    current_throughput=latest['throughput_ops_per_sec'],
                    avg_batch_latency_us=latest['avg_latency_ns'] / 1000,  # Convert ns to μs
                    buffer_utilization_percent=(latest['buffer_size'] / latest['buffer_capacity']) * 100,
                    network_stats={},  # Will be populated from system output
                    last_update=datetime.fromtimestamp(latest['timestamp_ns'] / 1_000_000_000)
                )
                
                print(f"Updated metrics: {latest['throughput_ops_per_sec']:.2f} ops/sec, "
                      f"latency: {latest['avg_latency_ns']/1000:.2f}μs")
            
            # Wait before next collection
            await asyncio.sleep(1)
            
        except Exception as e:
            print(f"Error in metrics collector: {e}")
            await asyncio.sleep(5)

@app.on_event("startup")
async def startup_event():
    """Start background metrics collection on startup."""
    asyncio.create_task(background_metrics_collector())

@app.on_event("shutdown")
async def shutdown_event():
    """Stop background metrics collection on shutdown."""
    global is_running
    is_running = False

@app.get("/")
async def root():
    """Root endpoint with service information."""
    return {
        "service": "Low-Latency Trading Telemetry",
        "version": "1.0.0",
        "endpoints": {
            "/metrics": "Prometheus metrics",
            "/latest": "Latest system statistics",
            "/health": "Health check"
        }
    }

@app.get("/metrics", response_class=PlainTextResponse)
async def metrics():
    """Prometheus metrics endpoint."""
    return generate_latest()

@app.get("/latest")
async def latest():
    """Get latest system statistics in JSON format."""
    if latest_stats is None:
        raise HTTPException(status_code=503, detail="No metrics available yet")
    return latest_stats

@app.get("/health")
async def health():
    """Health check endpoint."""
    return {
        "status": "healthy",
        "timestamp": datetime.now(),
        "metrics_collector_running": is_running,
        "telemetry_file_exists": telemetry_file_path.exists()
    }

@app.get("/stats/history")
async def stats_history(limit: int = 100):
    """Get historical statistics (simplified - reads from memory)."""
    # In a real implementation, this would read from a database
    # For now, return the latest stats
    if latest_stats is None:
        return []
    
    return [latest_stats]

if __name__ == "__main__":
    print("Starting Low-Latency Trading Telemetry Service...")
    print(f"Telemetry file: {telemetry_file_path}")
    print("Available endpoints:")
    print("  - GET /metrics (Prometheus format)")
    print("  - GET /latest (JSON format)")
    print("  - GET /health (Health check)")
    
    uvicorn.run(
        app,
        host="0.0.0.0",
        port=8000,
        log_level="info"
    ) 