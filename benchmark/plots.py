#!/usr/bin/env python3
"""
Benchmark analysis and visualization tool for low-latency trading system.
Loads CSV metrics and generates performance plots for latency, throughput, and batch analysis.
"""

import pandas as pd
import matplotlib.pyplot as plt
import seaborn as sns
import numpy as np
from pathlib import Path
import argparse
from datetime import datetime
import warnings
warnings.filterwarnings('ignore')

# Set style for better-looking plots
plt.style.use('seaborn-v0_8')
sns.set_palette("husl")

class BenchmarkAnalyzer:
    """Analyzes trading system performance metrics and generates visualizations."""
    
    def __init__(self, csv_file: str):
        """Initialize analyzer with CSV file path."""
        self.csv_file = Path(csv_file)
        self.df = None
        self.load_data()
        
    def load_data(self):
        """Load and preprocess CSV data."""
        try:
            # Expected CSV columns
            columns = [
                'timestamp_ns', 'orders_produced', 'orders_consumed', 'orders_dropped',
                'buffer_size', 'buffer_capacity', 'throughput_ops_per_sec',
                'avg_latency_ns', 'p95_latency_ns', 'p99_latency_ns',
                'network_errors', 'batch_count'
            ]
            
            # Load CSV data
            self.df = pd.read_csv(self.csv_file)
            
            # Ensure all expected columns exist
            for col in columns:
                if col not in self.df.columns:
                    print(f"Warning: Column '{col}' not found in CSV")
            
            # Convert timestamp to datetime
            if 'timestamp_ns' in self.df.columns:
                self.df['timestamp'] = pd.to_datetime(self.df['timestamp_ns'], unit='ns')
                self.df['time_seconds'] = (self.df['timestamp'] - self.df['timestamp'].min()).dt.total_seconds()
            
            # Convert latency from nanoseconds to microseconds for readability
            latency_cols = ['avg_latency_ns', 'p95_latency_ns', 'p99_latency_ns']
            for col in latency_cols:
                if col in self.df.columns:
                    self.df[col.replace('_ns', '_us')] = self.df[col] / 1000
            
            print(f"✅ Loaded {len(self.df)} data points from {self.csv_file}")
            print(f"   Time range: {self.df['timestamp'].min()} to {self.df['timestamp'].max()}")
            
        except Exception as e:
            print(f"❌ Error loading data: {e}")
            raise
    
    def plot_latency_histogram(self, save_path: str = "latency_histogram.png"):
        """Generate latency histogram plot."""
        # Ensure output directory exists
        Path(save_path).parent.mkdir(parents=True, exist_ok=True)
        
        fig, axes = plt.subplots(2, 2, figsize=(15, 12))
        fig.suptitle('Latency Distribution Analysis', fontsize=16, fontweight='bold')
        
        # Average latency histogram
        if 'avg_latency_us' in self.df.columns:
            axes[0, 0].hist(self.df['avg_latency_us'], bins=50, alpha=0.7, color='skyblue', edgecolor='black')
            axes[0, 0].set_title('Average Latency Distribution')
            axes[0, 0].set_xlabel('Latency (μs)')
            axes[0, 0].set_ylabel('Frequency')
            axes[0, 0].axvline(self.df['avg_latency_us'].mean(), color='red', linestyle='--', 
                              label=f'Mean: {self.df["avg_latency_us"].mean():.2f} μs')
            axes[0, 0].legend()
            
            # Add statistics text
            stats_text = f"Mean: {self.df['avg_latency_us'].mean():.2f} μs\n"
            stats_text += f"Std: {self.df['avg_latency_us'].std():.2f} μs\n"
            stats_text += f"Min: {self.df['avg_latency_us'].min():.2f} μs\n"
            stats_text += f"Max: {self.df['avg_latency_us'].max():.2f} μs"
            axes[0, 0].text(0.02, 0.98, stats_text, transform=axes[0, 0].transAxes, 
                           verticalalignment='top', bbox=dict(boxstyle='round', facecolor='wheat', alpha=0.8))
        
        # P95 latency histogram
        if 'p95_latency_us' in self.df.columns:
            axes[0, 1].hist(self.df['p95_latency_us'], bins=50, alpha=0.7, color='lightgreen', edgecolor='black')
            axes[0, 1].set_title('P95 Latency Distribution')
            axes[0, 1].set_xlabel('Latency (μs)')
            axes[0, 1].set_ylabel('Frequency')
            axes[0, 1].axvline(self.df['p95_latency_us'].mean(), color='red', linestyle='--',
                              label=f'Mean: {self.df["p95_latency_us"].mean():.2f} μs')
            axes[0, 1].legend()
        
        # P99 latency histogram
        if 'p99_latency_us' in self.df.columns:
            axes[1, 0].hist(self.df['p99_latency_us'], bins=50, alpha=0.7, color='lightcoral', edgecolor='black')
            axes[1, 0].set_title('P99 Latency Distribution')
            axes[1, 0].set_xlabel('Latency (μs)')
            axes[1, 0].set_ylabel('Frequency')
            axes[1, 0].axvline(self.df['p99_latency_us'].mean(), color='red', linestyle='--',
                              label=f'Mean: {self.df["p99_latency_us"].mean():.2f} μs')
            axes[1, 0].legend()
        
        # Latency percentiles comparison
        if all(col in self.df.columns for col in ['avg_latency_us', 'p95_latency_us', 'p99_latency_us']):
            latency_data = [self.df['avg_latency_us'], self.df['p95_latency_us'], self.df['p99_latency_us']]
            axes[1, 1].boxplot(latency_data, labels=['Average', 'P95', 'P99'])
            axes[1, 1].set_title('Latency Percentiles Comparison')
            axes[1, 1].set_ylabel('Latency (μs)')
            axes[1, 1].grid(True, alpha=0.3)
        
        plt.tight_layout()
        plt.savefig(save_path, dpi=300, bbox_inches='tight')
        print(f"✅ Latency histogram saved to {save_path}")
        plt.show()
    
    def plot_throughput_over_time(self, save_path: str = "throughput_timeline.png"):
        """Generate throughput over time line chart."""
        # Ensure output directory exists
        Path(save_path).parent.mkdir(parents=True, exist_ok=True)
        
        fig, axes = plt.subplots(2, 2, figsize=(15, 12))
        fig.suptitle('Throughput Analysis Over Time', fontsize=16, fontweight='bold')
        
        # Main throughput timeline
        if 'throughput_ops_per_sec' in self.df.columns and 'time_seconds' in self.df.columns:
            axes[0, 0].plot(self.df['time_seconds'], self.df['throughput_ops_per_sec'], 
                           linewidth=2, color='blue', alpha=0.8)
            axes[0, 0].set_title('Throughput Over Time')
            axes[0, 0].set_xlabel('Time (seconds)')
            axes[0, 0].set_ylabel('Throughput (orders/sec)')
            axes[0, 0].grid(True, alpha=0.3)
            
            # Add moving average
            window_size = min(20, len(self.df) // 10)
            if window_size > 1:
                moving_avg = self.df['throughput_ops_per_sec'].rolling(window=window_size).mean()
                axes[0, 0].plot(self.df['time_seconds'], moving_avg, 
                               linewidth=3, color='red', alpha=0.8, label=f'{window_size}-point moving avg')
                axes[0, 0].legend()
            
            # Add statistics
            avg_throughput = self.df['throughput_ops_per_sec'].mean()
            max_throughput = self.df['throughput_ops_per_sec'].max()
            axes[0, 0].axhline(avg_throughput, color='green', linestyle='--', alpha=0.7,
                              label=f'Average: {avg_throughput:.0f} ops/sec')
            axes[0, 0].legend()
        
        # Orders produced/consumed over time
        if all(col in self.df.columns for col in ['orders_produced', 'orders_consumed', 'time_seconds']):
            axes[0, 1].plot(self.df['time_seconds'], self.df['orders_produced'], 
                           linewidth=2, color='green', alpha=0.8, label='Orders Produced')
            axes[0, 1].plot(self.df['time_seconds'], self.df['orders_consumed'], 
                           linewidth=2, color='orange', alpha=0.8, label='Orders Consumed')
            axes[0, 1].set_title('Orders Produced vs Consumed')
            axes[0, 1].set_xlabel('Time (seconds)')
            axes[0, 1].set_ylabel('Cumulative Orders')
            axes[0, 1].legend()
            axes[0, 1].grid(True, alpha=0.3)
        
        # Buffer utilization over time
        if all(col in self.df.columns for col in ['buffer_size', 'buffer_capacity', 'time_seconds']):
            buffer_utilization = (self.df['buffer_size'] / self.df['buffer_capacity']) * 100
            axes[1, 0].plot(self.df['time_seconds'], buffer_utilization, 
                           linewidth=2, color='purple', alpha=0.8)
            axes[1, 0].set_title('Buffer Utilization Over Time')
            axes[1, 0].set_xlabel('Time (seconds)')
            axes[1, 0].set_ylabel('Buffer Utilization (%)')
            axes[1, 0].grid(True, alpha=0.3)
            axes[1, 0].axhline(buffer_utilization.mean(), color='red', linestyle='--', alpha=0.7,
                              label=f'Average: {buffer_utilization.mean():.1f}%')
            axes[1, 0].legend()
        
        # Batch count over time
        if 'batch_count' in self.df.columns and 'time_seconds' in self.df.columns:
            axes[1, 1].plot(self.df['time_seconds'], self.df['batch_count'], 
                           linewidth=2, color='brown', alpha=0.8)
            axes[1, 1].set_title('Cumulative Batches Sent')
            axes[1, 1].set_xlabel('Time (seconds)')
            axes[1, 1].set_ylabel('Total Batches')
            axes[1, 1].grid(True, alpha=0.3)
        
        plt.tight_layout()
        plt.savefig(save_path, dpi=300, bbox_inches='tight')
        print(f"✅ Throughput timeline saved to {save_path}")
        plt.show()
    
    def plot_performance_summary(self, save_path: str = "performance_summary.png"):
        """Generate comprehensive performance summary dashboard."""
        # Ensure output directory exists
        Path(save_path).parent.mkdir(parents=True, exist_ok=True)
        
        fig = plt.figure(figsize=(20, 16))
        fig.suptitle('Low-Latency Trading System Performance Summary', fontsize=20, fontweight='bold')
        
        # Create grid layout
        gs = fig.add_gridspec(4, 4, hspace=0.3, wspace=0.3)
        
        # 1. Key metrics summary (top left)
        ax1 = fig.add_subplot(gs[0, :2])
        if 'throughput_ops_per_sec' in self.df.columns:
            avg_throughput = self.df['throughput_ops_per_sec'].mean()
            max_throughput = self.df['throughput_ops_per_sec'].max()
            min_throughput = self.df['throughput_ops_per_sec'].min()
            
            metrics_text = f"PERFORMANCE SUMMARY\n\n"
            metrics_text += f"Throughput:\n"
            metrics_text += f"  Average: {avg_throughput:,.0f} ops/sec\n"
            metrics_text += f"  Maximum: {max_throughput:,.0f} ops/sec\n"
            metrics_text += f"  Minimum: {min_throughput:,.0f} ops/sec\n\n"
            
            if 'avg_latency_us' in self.df.columns:
                avg_latency = self.df['avg_latency_us'].mean()
                p95_latency = self.df['p95_latency_us'].mean() if 'p95_latency_us' in self.df.columns else 0
                p99_latency = self.df['p99_latency_us'].mean() if 'p99_latency_us' in self.df.columns else 0
                
                metrics_text += f"Latency:\n"
                metrics_text += f"  Average: {avg_latency:.2f} μs\n"
                metrics_text += f"  P95: {p95_latency:.2f} μs\n"
                metrics_text += f"  P99: {p99_latency:.2f} μs\n\n"
            
            if all(col in self.df.columns for col in ['orders_produced', 'orders_consumed']):
                total_produced = self.df['orders_produced'].iloc[-1]
                total_consumed = self.df['orders_consumed'].iloc[-1]
                drop_rate = ((total_produced - total_consumed) / total_produced) * 100 if total_produced > 0 else 0
                
                metrics_text += f"Orders:\n"
                metrics_text += f"  Produced: {total_produced:,}\n"
                metrics_text += f"  Consumed: {total_consumed:,}\n"
                metrics_text += f"  Drop Rate: {drop_rate:.2f}%\n\n"
            
            if 'batch_count' in self.df.columns:
                total_batches = self.df['batch_count'].iloc[-1]
                metrics_text += f"Batches: {total_batches:,}"
            
            ax1.text(0.05, 0.95, metrics_text, transform=ax1.transAxes, fontsize=12,
                    verticalalignment='top', bbox=dict(boxstyle='round', facecolor='lightblue', alpha=0.8))
            ax1.set_xlim(0, 1)
            ax1.set_ylim(0, 1)
            ax1.axis('off')
        
        # 2. Throughput distribution (top right)
        ax2 = fig.add_subplot(gs[0, 2:])
        if 'throughput_ops_per_sec' in self.df.columns:
            ax2.hist(self.df['throughput_ops_per_sec'], bins=30, alpha=0.7, color='skyblue', edgecolor='black')
            ax2.set_title('Throughput Distribution', fontweight='bold')
            ax2.set_xlabel('Throughput (orders/sec)')
            ax2.set_ylabel('Frequency')
            ax2.axvline(self.df['throughput_ops_per_sec'].mean(), color='red', linestyle='--', 
                       label=f'Mean: {self.df["throughput_ops_per_sec"].mean():.0f}')
            ax2.legend()
        
        # 3. Latency timeline (middle left)
        ax3 = fig.add_subplot(gs[1, :2])
        if all(col in self.df.columns for col in ['avg_latency_us', 'p95_latency_us', 'p99_latency_us', 'time_seconds']):
            ax3.plot(self.df['time_seconds'], self.df['avg_latency_us'], 
                    linewidth=2, color='blue', alpha=0.8, label='Average')
            ax3.plot(self.df['time_seconds'], self.df['p95_latency_us'], 
                    linewidth=2, color='orange', alpha=0.8, label='P95')
            ax3.plot(self.df['time_seconds'], self.df['p99_latency_us'], 
                    linewidth=2, color='red', alpha=0.8, label='P99')
            ax3.set_title('Latency Percentiles Over Time', fontweight='bold')
            ax3.set_xlabel('Time (seconds)')
            ax3.set_ylabel('Latency (μs)')
            ax3.legend()
            ax3.grid(True, alpha=0.3)
        
        # 4. Buffer utilization (middle right)
        ax4 = fig.add_subplot(gs[1, 2:])
        if all(col in self.df.columns for col in ['buffer_size', 'buffer_capacity', 'time_seconds']):
            buffer_utilization = (self.df['buffer_size'] / self.df['buffer_capacity']) * 100
            ax4.plot(self.df['time_seconds'], buffer_utilization, 
                    linewidth=2, color='purple', alpha=0.8)
            ax4.set_title('Buffer Utilization Over Time', fontweight='bold')
            ax4.set_xlabel('Time (seconds)')
            ax4.set_ylabel('Utilization (%)')
            ax4.grid(True, alpha=0.3)
        
        # 5. Orders timeline (bottom left)
        ax5 = fig.add_subplot(gs[2, :2])
        if all(col in self.df.columns for col in ['orders_produced', 'orders_consumed', 'time_seconds']):
            ax5.plot(self.df['time_seconds'], self.df['orders_produced'], 
                    linewidth=2, color='green', alpha=0.8, label='Produced')
            ax5.plot(self.df['time_seconds'], self.df['orders_consumed'], 
                    linewidth=2, color='orange', alpha=0.8, label='Consumed')
            ax5.set_title('Cumulative Orders Over Time', fontweight='bold')
            ax5.set_xlabel('Time (seconds)')
            ax5.set_ylabel('Orders')
            ax5.legend()
            ax5.grid(True, alpha=0.3)
        
        # 6. Network errors (bottom right)
        ax6 = fig.add_subplot(gs[2, 2:])
        if 'network_errors' in self.df.columns and 'time_seconds' in self.df.columns:
            ax6.plot(self.df['time_seconds'], self.df['network_errors'], 
                    linewidth=2, color='red', alpha=0.8)
            ax6.set_title('Network Errors Over Time', fontweight='bold')
            ax6.set_xlabel('Time (seconds)')
            ax6.set_ylabel('Errors')
            ax6.grid(True, alpha=0.3)
        
        # 7. Performance correlation (bottom)
        ax7 = fig.add_subplot(gs[3, :])
        if all(col in self.df.columns for col in ['throughput_ops_per_sec', 'avg_latency_us']):
            ax7.scatter(self.df['throughput_ops_per_sec'], self.df['avg_latency_us'], 
                       alpha=0.6, color='blue', s=20)
            ax7.set_title('Throughput vs Latency Correlation', fontweight='bold')
            ax7.set_xlabel('Throughput (orders/sec)')
            ax7.set_ylabel('Average Latency (μs)')
            ax7.grid(True, alpha=0.3)
            
            # Add correlation coefficient
            correlation = self.df['throughput_ops_per_sec'].corr(self.df['avg_latency_us'])
            ax7.text(0.02, 0.98, f'Correlation: {correlation:.3f}', 
                    transform=ax7.transAxes, verticalalignment='top',
                    bbox=dict(boxstyle='round', facecolor='yellow', alpha=0.8))
        
        plt.savefig(save_path, dpi=300, bbox_inches='tight')
        print(f"✅ Performance summary saved to {save_path}")
        plt.show()
    
    def generate_report(self, output_dir: str = "benchmark_results"):
        """Generate comprehensive benchmark report with all plots."""
        output_path = Path(output_dir)
        output_path.mkdir(exist_ok=True)
        
        print(f"📊 Generating benchmark report in {output_path}")
        
        # Generate all plots
        self.plot_latency_histogram(output_path / "latency_histogram.png")
        self.plot_throughput_over_time(output_path / "throughput_timeline.png")
        self.plot_performance_summary(output_path / "performance_summary.png")
        
        # Generate statistics report
        self.generate_statistics_report(output_path / "statistics.txt")
        
        print(f"✅ Benchmark report generated in {output_path}")
        print(f"   📈 latency_histogram.png")
        print(f"   📈 throughput_timeline.png") 
        print(f"   📈 performance_summary.png")
        print(f"   📄 statistics.txt")
    
    def generate_statistics_report(self, output_file: str):
        """Generate detailed statistics report."""
        with open(output_file, 'w') as f:
            f.write("LOW-LATENCY TRADING SYSTEM BENCHMARK REPORT\n")
            f.write("=" * 50 + "\n\n")
            f.write(f"Generated: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}\n")
            f.write(f"Data points: {len(self.df)}\n")
            f.write(f"Time range: {self.df['timestamp'].min()} to {self.df['timestamp'].max()}\n\n")
            
            # Throughput statistics
            if 'throughput_ops_per_sec' in self.df.columns:
                f.write("THROUGHPUT STATISTICS\n")
                f.write("-" * 20 + "\n")
                f.write(f"Average: {self.df['throughput_ops_per_sec'].mean():.2f} ops/sec\n")
                f.write(f"Maximum: {self.df['throughput_ops_per_sec'].max():.2f} ops/sec\n")
                f.write(f"Minimum: {self.df['throughput_ops_per_sec'].min():.2f} ops/sec\n")
                f.write(f"Std Dev: {self.df['throughput_ops_per_sec'].std():.2f} ops/sec\n\n")
            
            # Latency statistics
            if 'avg_latency_us' in self.df.columns:
                f.write("LATENCY STATISTICS (μs)\n")
                f.write("-" * 25 + "\n")
                f.write(f"Average: {self.df['avg_latency_us'].mean():.2f}\n")
                f.write(f"P50: {self.df['avg_latency_us'].median():.2f}\n")
                if 'p95_latency_us' in self.df.columns:
                    f.write(f"P95: {self.df['p95_latency_us'].mean():.2f}\n")
                if 'p99_latency_us' in self.df.columns:
                    f.write(f"P99: {self.df['p99_latency_us'].mean():.2f}\n")
                f.write(f"Std Dev: {self.df['avg_latency_us'].std():.2f}\n\n")
            
            # Order statistics
            if all(col in self.df.columns for col in ['orders_produced', 'orders_consumed']):
                total_produced = self.df['orders_produced'].iloc[-1]
                total_consumed = self.df['orders_consumed'].iloc[-1]
                drop_rate = ((total_produced - total_consumed) / total_produced) * 100 if total_produced > 0 else 0
                
                f.write("ORDER STATISTICS\n")
                f.write("-" * 15 + "\n")
                f.write(f"Total Produced: {total_produced:,}\n")
                f.write(f"Total Consumed: {total_consumed:,}\n")
                f.write(f"Drop Rate: {drop_rate:.2f}%\n\n")
            
            # Buffer statistics
            if all(col in self.df.columns for col in ['buffer_size', 'buffer_capacity']):
                buffer_utilization = (self.df['buffer_size'] / self.df['buffer_capacity']) * 100
                f.write("BUFFER STATISTICS\n")
                f.write("-" * 16 + "\n")
                f.write(f"Average Utilization: {buffer_utilization.mean():.2f}%\n")
                f.write(f"Maximum Utilization: {buffer_utilization.max():.2f}%\n")
                f.write(f"Buffer Capacity: {self.df['buffer_capacity'].iloc[0]:,}\n\n")
            
            # Network statistics
            if 'network_errors' in self.df.columns:
                total_errors = self.df['network_errors'].sum()
                f.write("NETWORK STATISTICS\n")
                f.write("-" * 17 + "\n")
                f.write(f"Total Errors: {total_errors}\n")
                f.write(f"Error Rate: {total_errors / len(self.df):.4f} errors/measurement\n\n")
        
        print(f"✅ Statistics report saved to {output_file}")

def main():
    """Main function to run benchmark analysis."""
    parser = argparse.ArgumentParser(description='Analyze trading system performance metrics')
    parser.add_argument('csv_file', help='Path to CSV metrics file')
    parser.add_argument('--output', '-o', default='benchmark_results', 
                       help='Output directory for plots (default: benchmark_results)')
    parser.add_argument('--latency-only', action='store_true', 
                       help='Generate only latency histogram')
    parser.add_argument('--throughput-only', action='store_true', 
                       help='Generate only throughput timeline')
    parser.add_argument('--summary-only', action='store_true', 
                       help='Generate only performance summary')
    
    args = parser.parse_args()
    
    # Initialize analyzer
    analyzer = BenchmarkAnalyzer(args.csv_file)
    
    # Generate requested plots
    if args.latency_only:
        analyzer.plot_latency_histogram(f"{args.output}/latency_histogram.png")
    elif args.throughput_only:
        analyzer.plot_throughput_over_time(f"{args.output}/throughput_timeline.png")
    elif args.summary_only:
        analyzer.plot_performance_summary(f"{args.output}/performance_summary.png")
    else:
        # Generate full report
        analyzer.generate_report(args.output)

if __name__ == "__main__":
    main() 