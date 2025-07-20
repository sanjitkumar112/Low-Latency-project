# Low-Latency Trading Telemetry Service

A FastAPI-based service that reads telemetry data from the low-latency trading system and exposes it as a real-time API with Prometheus metrics.

## Features

- **Real-time metrics collection** from CSV telemetry logs
- **Prometheus-compatible metrics** endpoint (`/metrics`)
- **JSON API** for latest statistics (`/latest`)
- **Health check** endpoint (`/health`)
- **Background data collection** with automatic updates

## Installation

1. Install Python dependencies:
```bash
pip install -r requirements.txt
```

2. Generate sample data (optional):
```bash
python generate_sample_data.py
```

## Usage

### Start the telemetry service:
```bash
python metrics.py
```

The service will start on `http://localhost:8000`

### Available Endpoints

- **GET /** - Service information and available endpoints
- **GET /metrics** - Prometheus-compatible metrics (text format)
- **GET /latest** - Latest system statistics (JSON format)
- **GET /health** - Health check and service status
- **GET /stats/history** - Historical statistics (simplified)

### Example API Calls

```bash
# Get service info
curl http://localhost:8000/

# Get Prometheus metrics
curl http://localhost:8000/metrics

# Get latest stats
curl http://localhost:8000/latest

# Health check
curl http://localhost:8000/health
```

## Prometheus Metrics

The service exposes the following Prometheus metrics:

### Counters
- `orders_produced_total` - Total orders produced (labeled by producer_id)
- `orders_consumed_total` - Total orders consumed (labeled by consumer_id)
- `batches_sent_total` - Total batches sent
- `network_dropped_packets_total` - Network dropped packets (labeled by network_type)
- `network_retransmissions_total` - Network retransmissions (labeled by network_type)

### Gauges
- `throughput_orders_per_second` - Current throughput
- `buffer_utilization_percent` - Ring buffer utilization

### Histograms
- `batch_latency_seconds` - Batch processing latency distribution

## Telemetry Data Format

The service expects telemetry data in CSV format with the following columns:

```csv
timestamp_ns,orders_produced,orders_consumed,orders_dropped,buffer_size,buffer_capacity,throughput_ops_per_sec,avg_latency_ns,p95_latency_ns,p99_latency_ns,network_errors,batch_count
```

### Example:
```csv
1703123456789000000,10000,9950,50,512,1024,8500.5,250000,375000,500000,2,150
```

## Integration with Trading System

The telemetry service reads from `telemetry.log` in the project root. To integrate with the C++ trading system:

1. Modify the C++ system to write telemetry data in the expected CSV format
2. The Python service will automatically detect and read new data
3. Metrics are updated in real-time as new data arrives

## Configuration

- **Port**: Default 8000 (configurable in `metrics.py`)
- **Host**: Default 0.0.0.0 (all interfaces)
- **Telemetry file**: `telemetry.log` in project root
- **Update interval**: 1 second

## Monitoring

### Prometheus Integration
Add to your `prometheus.yml`:
```yaml
scrape_configs:
  - job_name: 'trading-telemetry'
    static_configs:
      - targets: ['localhost:8000']
    metrics_path: '/metrics'
    scrape_interval: 5s
```

### Grafana Dashboard
Create dashboards using the exposed metrics:
- Throughput over time
- Latency percentiles
- Buffer utilization
- Network error rates

## Development

### Running in Development Mode
```bash
uvicorn metrics:app --reload --host 0.0.0.0 --port 8000
```

### Testing with Sample Data
```bash
# Generate sample data
python generate_sample_data.py

# Start service
python metrics.py

# Test endpoints
curl http://localhost:8000/latest
curl http://localhost:8000/metrics
```

## Architecture

```
┌─────────────────┐    ┌──────────────────┐    ┌─────────────────┐
│   C++ Trading   │───▶│   telemetry.log  │───▶│  Python Service │
│     System      │    │     (CSV)        │    │   (FastAPI)     │
└─────────────────┘    └──────────────────┘    └─────────────────┘
                                                          │
                                                          ▼
                                               ┌─────────────────┐
                                               │   Prometheus    │
                                               │   /metrics      │
                                               └─────────────────┘
                                                          │
                                                          ▼
                                               ┌─────────────────┐
                                               │   JSON API      │
                                               │   /latest       │
                                               └─────────────────┘
```

## Troubleshooting

### Service not starting
- Check if port 8000 is available
- Verify Python dependencies are installed
- Check telemetry.log file exists

### No metrics data
- Verify telemetry.log has the correct CSV format
- Check file permissions
- Look for parsing errors in service logs

### High memory usage
- The service keeps latest stats in memory
- Consider implementing data retention policies
- Monitor background collector performance 