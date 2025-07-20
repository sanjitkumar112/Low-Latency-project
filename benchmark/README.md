# Benchmark Analysis Tool

A comprehensive performance analysis and visualization tool for low-latency trading systems. Loads CSV metrics and generates professional-grade performance plots and reports.

## 🎯 Features

### 📊 **Visualization Types**
- **Latency Histograms**: Distribution analysis of average, P95, and P99 latencies
- **Throughput Timeline**: Real-time throughput over time with moving averages
- **Performance Summary**: Comprehensive dashboard with all key metrics
- **Buffer Utilization**: Memory usage patterns over time
- **Network Error Analysis**: Error rates and patterns

### 📈 **Generated Plots**
- `latency_histogram.png` - Latency distribution analysis
- `throughput_timeline.png` - Throughput and order flow over time
- `performance_summary.png` - Complete performance dashboard
- `statistics.txt` - Detailed numerical analysis

### 🔍 **Metrics Analyzed**
- **Throughput**: Average, max, min, standard deviation
- **Latency**: Average, P50, P95, P99 percentiles
- **Orders**: Production/consumption rates, drop rates
- **Buffer**: Utilization patterns and capacity analysis
- **Network**: Error rates and correlation analysis

## 🚀 Quick Start

### Installation
```bash
pip3 install -r requirements.txt
```

### Basic Usage
```bash
# Generate all plots and reports
python3 plots.py telemetry.log

# Generate specific plots only
python3 plots.py telemetry.log --latency-only
python3 plots.py telemetry.log --throughput-only
python3 plots.py telemetry.log --summary-only

# Custom output directory
python3 plots.py telemetry.log --output my_results
```

## 📋 Expected CSV Format

The tool expects telemetry data in the following CSV format:

```csv
timestamp_ns,orders_produced,orders_consumed,orders_dropped,buffer_size,buffer_capacity,throughput_ops_per_sec,avg_latency_ns,p95_latency_ns,p99_latency_ns,network_errors,batch_count
1753045057422138112,3892,3846,9,80,1024,10799.56,468784.09,973816.42,1398182.26,4,80
```

### Required Columns
- `timestamp_ns` - Nanosecond timestamp
- `orders_produced` - Cumulative orders produced
- `orders_consumed` - Cumulative orders consumed
- `throughput_ops_per_sec` - Current throughput
- `avg_latency_ns` - Average latency in nanoseconds

### Optional Columns
- `p95_latency_ns` - 95th percentile latency
- `p99_latency_ns` - 99th percentile latency
- `buffer_size` - Current buffer usage
- `buffer_capacity` - Buffer capacity
- `network_errors` - Network error count
- `batch_count` - Cumulative batches sent

## 📊 Sample Results

### Performance Summary
```
THROUGHPUT STATISTICS
Average: 9,794 ops/sec
Maximum: 11,961 ops/sec
Minimum: 8,016 ops/sec

LATENCY STATISTICS (μs)
Average: 312.81
P95: 622.77
P99: 1023.40

ORDER STATISTICS
Total Produced: 288,068
Drop Rate: 0.85%

BUFFER STATISTICS
Average Utilization: 50.04%
Maximum Utilization: 98.14%
```

## 🎨 Plot Types

### 1. Latency Histogram
- **Average Latency Distribution**: Shows the spread of average latencies
- **P95 Latency Distribution**: 95th percentile latency patterns
- **P99 Latency Distribution**: 99th percentile latency patterns
- **Percentile Comparison**: Box plot comparing all latency percentiles

### 2. Throughput Timeline
- **Throughput Over Time**: Real-time throughput with moving averages
- **Orders Timeline**: Cumulative orders produced vs consumed
- **Buffer Utilization**: Memory usage patterns over time
- **Batch Count**: Cumulative batches sent over time

### 3. Performance Summary Dashboard
- **Key Metrics Summary**: Overview of all performance indicators
- **Throughput Distribution**: Histogram of throughput values
- **Latency Timeline**: All percentiles over time
- **Buffer Analysis**: Utilization patterns
- **Order Flow**: Production vs consumption
- **Network Errors**: Error patterns over time
- **Correlation Analysis**: Throughput vs latency relationship

## 🔧 Advanced Usage

### Command Line Options
```bash
python3 plots.py --help
```

Available options:
- `csv_file` - Path to CSV metrics file (required)
- `--output, -o` - Output directory (default: benchmark_results)
- `--latency-only` - Generate only latency histogram
- `--throughput-only` - Generate only throughput timeline
- `--summary-only` - Generate only performance summary

### Programmatic Usage
```python
from plots import BenchmarkAnalyzer

# Initialize analyzer
analyzer = BenchmarkAnalyzer('telemetry.log')

# Generate specific plots
analyzer.plot_latency_histogram('my_latency.png')
analyzer.plot_throughput_over_time('my_throughput.png')
analyzer.plot_performance_summary('my_summary.png')

# Generate complete report
analyzer.generate_report('my_results')
```

## 📈 Integration with Trading System

### 1. Data Collection
Ensure your C++ trading system writes telemetry data in the expected CSV format to a file.

### 2. Periodic Analysis
Run the benchmark tool periodically to analyze performance:
```bash
# Daily analysis
python3 plots.py daily_telemetry.log --output daily_reports/$(date +%Y-%m-%d)

# Weekly summary
python3 plots.py weekly_telemetry.log --output weekly_reports/
```

### 3. Performance Monitoring
Use the generated plots to:
- Identify performance bottlenecks
- Monitor latency trends
- Track throughput improvements
- Analyze buffer utilization patterns
- Detect network issues

## 🎯 Use Cases

### Performance Optimization
- Identify latency spikes and their causes
- Analyze throughput patterns under different loads
- Monitor buffer utilization to optimize memory usage
- Track performance improvements over time

### Capacity Planning
- Understand system limits under various conditions
- Plan for scaling based on throughput patterns
- Analyze resource utilization trends

### Quality Assurance
- Ensure consistent performance across releases
- Detect performance regressions
- Validate optimization effectiveness

### Reporting
- Generate performance reports for stakeholders
- Create visualizations for presentations
- Document system performance characteristics

## 📁 File Structure

```
benchmark/
├── plots.py              # Main analysis tool
├── requirements.txt      # Python dependencies
├── README.md            # This file
└── benchmark_results/   # Generated outputs
    ├── latency_histogram.png
    ├── throughput_timeline.png
    ├── performance_summary.png
    └── statistics.txt
```

## 🔍 Troubleshooting

### Common Issues

**"Column not found" warnings**
- Ensure your CSV has the expected column names
- Check for typos in column headers
- The tool will work with missing optional columns

**Empty plots**
- Verify your CSV file contains data
- Check that numeric columns contain valid numbers
- Ensure timestamps are in nanoseconds

**Memory issues with large files**
- Consider processing data in chunks
- Use `--latency-only` or `--throughput-only` for specific analysis
- Reduce plot resolution if needed

## 🚀 Future Enhancements

Potential improvements:
- **Real-time monitoring**: Live plot updates
- **Comparative analysis**: Compare multiple runs
- **Statistical significance**: Confidence intervals
- **Export options**: PDF, SVG, interactive HTML
- **Custom metrics**: User-defined performance indicators
- **Alerting**: Automatic performance alerts

---

**Status**: ✅ **Production Ready** 