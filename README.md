# Low-Latency Trading System

This project is a simple, high-performance trading system designed to demonstrate low-latency order processing using C++ and Python.

I built this project as my first deep dive into systems programming and real-world software engineering. My goal was to understand how high-frequency trading systems achieve ultra-low latency and high throughput, and to learn how to design software that is both fast and reliable.

**Thought Process & Design Journey:**
- I started by researching how trading systems work, especially the importance of minimizing delays between when an order is created and when it is processed.
- To achieve this, I implemented a **lock-free ring buffer** in C++. This allows producer and consumer threads to communicate without blocking each other, which is crucial for speed.
- I wanted to see how batching orders could improve efficiency, so I added a **batch processor** that groups orders before sending them for execution.
- Real trading systems face unpredictable network conditions, so I included **network simulation modules** (TCP, UDP, shared memory) to test how the system performs under different scenarios.
- To make the system observable, I built a **real-time telemetry dashboard** using Python and FastAPI. This lets me (and others) monitor performance metrics live, just like in professional environments.
- Finally, I created a **benchmarking tool** in Python to analyze and visualize the system's performance, helping me identify bottlenecks and optimize further.

Throughout the project, I focused on writing clean, modular code and documenting my process, so others can learn from my journey or use this as a starting point for their own experiments.

## Features
- **Lock-free ring buffer** for fast, thread-safe communication
- **Batch order processing** for efficiency
- **Network simulation** (TCP, UDP, shared memory)
- **Real-time telemetry dashboard** (Python FastAPI)
- **Performance analysis** (Python benchmarking tool)

## Getting Started

### Prerequisites
- C++17 compiler (e.g., g++)
- Python 3.8+
- Python packages: `fastapi`, `uvicorn`, `prometheus-client`, `pandas`, `matplotlib`, `seaborn`, `numpy`

### Build and Run (C++)
```bash
make
./ring_buffer_demo
```

### Start Telemetry Service (Python)
```bash
cd telemetry
pip install -r requirements.txt
python3 metrics.py
```

### Run Benchmark Analysis (Python)
```bash
cd benchmark
pip install -r requirements.txt
python3 plots.py ../telemetry.log
```

### View Dashboard
Open your browser to: [http://localhost:8000](http://localhost:8000)

## Project Structure
- `main.cpp`, `order.hpp`, `ring_buffer.hpp`, `batcher.hpp`: C++ trading system
- `src/network_sim/`: Network simulation modules
- `telemetry/metrics.py`: Real-time metrics API
- `benchmark/plots.py`: Performance analysis tool

---

**For learning, experimenting, and exploring low-latency system design!** 