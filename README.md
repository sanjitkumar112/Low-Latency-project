# Low-Latency Trading System

A simple, efficient C++ project that demonstrates low-latency order processing using multi-threading, batching, and network simulation.

## Features
- Lock-free ring buffer for fast, thread-safe communication
- Batch order processing for efficiency
- Network simulation: choose TCP, UDP, or SHM (shared memory)
- Simple, clear statistics output at the end of each run

## Getting Started

### Prerequisites
- C++17 compiler (e.g., g++)
- `make` utility (for building)

### Build and Run

```bash
cd /Users/sanjitkumar/Documents/Projects/Low-Latency-project
make
./ring_buffer_demo
```

### Selecting Network Simulation Type

You can choose the network simulation mode (TCP, UDP, or SHM) by editing this line in `main.cpp`:

```cpp
Config cfg;
cfg.net_type = NetworkType::TCP; // Change to NetworkType::UDP or NetworkType::SHM
```

- `NetworkType::TCP` – Simulates reliable, congestion-controlled network
- `NetworkType::UDP` – Simulates fast, lossy network
- `NetworkType::SHM` – Simulates shared memory (very low latency)

Rebuild and run after making changes.

## Output
At the end of each run, you'll see a summary like:

```
=== Final Statistics ===
Total orders produced: ...
Total orders consumed: ...
Total batches sent: ...
Average batch latency: ...μs

=== [Network] Network Statistics ===
... (network-specific stats)
==============================
```

## Project Structure
- `main.cpp`, `order.hpp`, `ring_buffer.hpp`, `batcher.hpp`: C++ core logic
- `src/network_sim/`: Network simulation modules (TCP, UDP, SHM)

## Clean Up
- You can safely delete any `.png`, `.txt`, `.DS_Store`, `__pycache__`, or output files. Only the source code is needed to rebuild and rerun everything.

---

## Connect

Follow me on Twitter: [@sanjitkmr112](https://x.com/sanjitkmr112/status/1947681278679019638) 