# Local Viewing Guide

How to view and test the Low-Latency Trading System locally without AWS deployment.

## 🚀 Quick Start

### 1. **Start the Trading System**
```bash
# Build the C++ system
make clean && make

# Start the trading system (in background)
./ring_buffer_demo &
```

### 2. **Start the Telemetry Service**
```bash
# Start telemetry service (in background)
cd telemetry && python3 metrics.py &
```

### 3. **Generate Test Data**
```bash
# Generate sample telemetry data
python3 generate_test_data.py
```

### 4. **View the Dashboard**
Open your browser and go to:
```
http://localhost:8000
```

## 📊 Available Endpoints

### **Service Information**
```bash
curl http://localhost:8000/
```
**Response:**
```json
{
  "service": "Low-Latency Trading Telemetry",
  "version": "1.0.0",
  "endpoints": {
    "/metrics": "Prometheus metrics",
    "/latest": "Latest system statistics",
    "/health": "Health check"
  }
}
```

### **Health Check**
```bash
curl http://localhost:8000/health
```
**Response:**
```json
{
  "status": "healthy",
  "timestamp": "2025-07-20T16:33:43.718801",
  "metrics_collector_running": true,
  "telemetry_file_exists": true
}
```

### **Latest Metrics**
```bash
curl http://localhost:8000/latest
```
**Response:**
```json
{
  "total_orders_produced": 142151,
  "total_orders_consumed": 141160,
  "total_batches_sent": 6732,
  "current_throughput": 9927.79,
  "avg_batch_latency_us": 298.48,
  "buffer_utilization_percent": 53.12,
  "network_stats": {},
  "last_update": "2025-07-20T21:35:00.370344960"
}
```

### **Prometheus Metrics**
```bash
curl http://localhost:8000/metrics
```
**Response:** Prometheus-formatted metrics for monitoring systems

## 📈 Performance Analysis

### **Generate Benchmark Reports**
```bash
cd benchmark
python3 plots.py ../telemetry.log --output local_results
```

### **View Generated Reports**
```bash
# View statistics
cat local_results/statistics.txt

# Open plots in browser (if you have a local server)
python3 -m http.server 8080
# Then visit: http://localhost:8080/local_results/
```

### **Sample Performance Results**
```
THROUGHPUT STATISTICS
Average: 9,927 ops/sec
Maximum: 11,967 ops/sec
Minimum: 8,087 ops/sec

LATENCY STATISTICS (μs)
Average: 298.48
P95: 595.87
P99: 930.00

ORDER STATISTICS
Total Produced: 142,151
Drop Rate: 0.70%

BUFFER STATISTICS
Average Utilization: 53.12%
Maximum Utilization: 98.14%
```

## 🎯 Interactive Testing

### **Test Different Network Types**
The C++ system supports different network simulation modes:

```bash
# Run with TCP simulation
./ring_buffer_demo tcp

# Run with UDP simulation  
./ring_buffer_demo udp

# Run with shared memory simulation
./ring_buffer_demo shm
```

### **Monitor Real-time Performance**
```bash
# Watch telemetry data in real-time
tail -f telemetry.log

# Monitor system resources
htop

# Check network connections
netstat -tlnp | grep :8000
```

### **Generate Different Data Patterns**
Modify `generate_test_data.py` to create different scenarios:

```python
# High throughput scenario
throughput = random.uniform(15000, 20000)

# High latency scenario  
avg_latency_ns = random.uniform(500_000, 1000_000)

# High error rate scenario
network_errors = random.randint(10, 50)
```

## 🔧 Development Workflow

### **1. Make Changes**
```bash
# Edit C++ code
vim main.cpp

# Edit telemetry service
vim telemetry/metrics.py

# Edit benchmark tool
vim benchmark/plots.py
```

### **2. Test Changes**
```bash
# Rebuild C++ system
make clean && make

# Restart telemetry service
pkill -f metrics.py
cd telemetry && python3 metrics.py &

# Generate new test data
python3 generate_test_data.py
```

### **3. View Results**
```bash
# Check telemetry dashboard
curl http://localhost:8000/latest

# Generate new benchmark report
cd benchmark && python3 plots.py ../telemetry.log --output new_results
```

## 📁 File Locations

### **Generated Files**
- `telemetry.log` - Telemetry data (CSV format)
- `local_results/` - Benchmark analysis outputs
  - `latency_histogram.png` - Latency distribution
  - `throughput_timeline.png` - Throughput over time
  - `performance_summary.png` - Complete dashboard
  - `statistics.txt` - Numerical analysis

### **Service Logs**
- C++ trading system: Console output
- Telemetry service: Console output
- System logs: `tail -f /var/log/system.log`

## 🎨 Visualization Options

### **1. Web Dashboard**
- **URL**: http://localhost:8000
- **Features**: Real-time metrics, health checks, JSON API

### **2. Prometheus Integration**
- **URL**: http://localhost:8000/metrics
- **Use**: Connect to Prometheus/Grafana for advanced monitoring

### **3. Static Plots**
- **Location**: `local_results/`
- **Formats**: PNG images, CSV data, JSON statistics

### **4. Command Line**
```bash
# Real-time monitoring
watch -n 1 'curl -s http://localhost:8000/latest | jq'

# Performance summary
cd benchmark && python3 plots.py ../telemetry.log --summary-only
```

## 🔍 Debugging

### **Common Issues**

#### **Service Not Starting**
```bash
# Check if port 8000 is in use
lsof -i :8000

# Kill existing process
pkill -f metrics.py
```

#### **No Telemetry Data**
```bash
# Check if telemetry.log exists
ls -la telemetry.log

# Generate test data
python3 generate_test_data.py
```

#### **Build Errors**
```bash
# Check dependencies
which g++
which python3

# Clean and rebuild
make clean && make
```

### **Performance Issues**
```bash
# Check system resources
htop
free -h
df -h

# Monitor network
netstat -i
```

## 🎯 Next Steps

1. **Explore the Code**: Read through the source files
2. **Modify Parameters**: Adjust trading system settings
3. **Add Features**: Extend the system with new capabilities
4. **Performance Tuning**: Optimize for your specific use case
5. **Deploy to Cloud**: When ready, use the AWS deployment scripts

## 💡 Tips

- **Use tmux**: Keep services running in background sessions
- **Monitor Logs**: Watch console output for real-time feedback
- **Experiment**: Try different network types and parameters
- **Save Results**: Keep benchmark reports for comparison
- **Version Control**: Commit your changes to track improvements

---

**Status**: ✅ **Ready for Local Development** 