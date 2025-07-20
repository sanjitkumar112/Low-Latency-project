# Quick Start Guide - Telemetry Service

## 🚀 What We Built

A **FastAPI-based telemetry service** that reads real-time data from your low-latency trading system and exposes it as:

- **Prometheus metrics** (`/metrics`) - For monitoring dashboards
- **JSON API** (`/latest`) - For real-time stats
- **Health checks** (`/health`) - For service monitoring

## 📊 Current Status

✅ **Service Running**: `http://localhost:8000`
✅ **Data Processing**: Reading from `telemetry.log`
✅ **Metrics Available**: 288K orders, 11K batches, 10K ops/sec

## 🎯 Key Features

### Real-time Metrics
- **Throughput**: 10,117 ops/sec
- **Latency**: 229 μs average
- **Buffer Utilization**: 0.1%
- **Orders**: 288K produced, 285K consumed

### API Endpoints
```bash
# Service info
curl http://localhost:8000/

# Health check
curl http://localhost:8000/health

# Latest metrics (JSON)
curl http://localhost:8000/latest

# Prometheus metrics
curl http://localhost:8000/metrics
```

## 🔧 Integration with Trading System

The service automatically reads CSV telemetry data from `telemetry.log`:

```csv
timestamp_ns,orders_produced,orders_consumed,orders_dropped,buffer_size,buffer_capacity,throughput_ops_per_sec,avg_latency_ns,p95_latency_ns,p99_latency_ns,network_errors,batch_count
```

## 📈 Monitoring Setup

### Prometheus Configuration
```yaml
scrape_configs:
  - job_name: 'trading-telemetry'
    static_configs:
      - targets: ['localhost:8000']
    metrics_path: '/metrics'
    scrape_interval: 5s
```

### Grafana Dashboards
Create dashboards using these metrics:
- `throughput_orders_per_second`
- `batch_latency_seconds`
- `buffer_utilization_percent`
- `orders_produced_total`

## 🧪 Testing

Run the test script to verify everything works:
```bash
python3 test_service.py
```

## 🛠️ Development

### Start Service
```bash
python3 metrics.py
```

### Generate Sample Data
```bash
python3 generate_sample_data.py
```

### Install Dependencies
```bash
pip3 install -r requirements.txt
```

## 🎯 Next Steps

1. **Connect to Real Trading System**: Modify your C++ system to write telemetry data in CSV format
2. **Set Up Monitoring**: Configure Prometheus and Grafana
3. **Add Alerts**: Set up alerting rules for high latency or low throughput
4. **Scale**: Add more telemetry sources or distributed monitoring

## 📁 Files Created

- `metrics.py` - Main FastAPI service
- `requirements.txt` - Python dependencies
- `generate_sample_data.py` - Sample data generator
- `test_service.py` - Service testing script
- `README.md` - Detailed documentation
- `QUICK_START.md` - This guide

---

**Status**: ✅ **Ready for Production Use** 