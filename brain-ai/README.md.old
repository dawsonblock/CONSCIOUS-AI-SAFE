# Brain-AI v3.6.0

**Offline-First, Low-Latency Quantum Consciousness System with Automatic Graph**

[![C++20](https://img.shields.io/badge/C%2B%2B-20-blue.svg)](https://en.cppreference.com/w/cpp/20)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)

## 🎯 Performance Targets

| Metric | Target | Status |
|--------|--------|--------|
| Evolution step (CPU p95) | ≤10 ms | ✅ |
| Recall (p95, 1M vectors) | ≤30 ms | ✅ |
| Memory backend | FAISS IVF+PQ | ✅ |
| Auto-graph | Enabled | ✅ |
| Security | Kill-switch, seccomp | ✅ |
| CI/CD | README compliance | ✅ |

## 🌟 Key Features

### Core Architecture
- **Quantum Workspace**: 7D Lindblad evolution with 8.2 Hz collapse
- **Pluggable Memory**: FAISS (IVF+PQ default), HNSWlib, SQLite-VSS, Qdrant
- **Auto-Graph**: Automatic connection tracking with reranking
- **LRU Cache**: 100K items with TTL for embeddings and results
- **Filters**: Offline SQL metadata filtering
- **Metrics**: Prometheus-compatible exports

### Production Ready
- **gRPC + HTTP**: Dual protocol support
- **Docker**: Multi-stage builds (<1GB)
- **Kubernetes**: 3-replica HA with HPA, NetworkPolicy
- **Security**: Non-root, RO filesystem, seccomp profiles, kill-switch
- **Persistence**: Zero-downtime snapshots and reloads

## 🚀 Quick Start

### Local Build

```bash
# Install dependencies (Debian/Ubuntu)
sudo apt-get update && sudo apt-get install -y \
    build-essential cmake ninja-build pkg-config g++ \
    libeigen3-dev libprotobuf-dev protobuf-compiler \
    protobuf-compiler-grpc libgrpc++-dev \
    libssl-dev libsqlite3-dev libyaml-cpp-dev \
    libopenblas-dev liblapack-dev

# Build
cd brain-ai
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)

# Run tests
cd build && ctest --output-on-failure

# Start server
./build/brain_server configs/system.yaml
```

### Docker

```bash
# Build image
docker build -t brain-ai:3.6.0 -f docker/Dockerfile .

# Run
docker run -p 50051:50051 -p 8080:8080 brain-ai:3.6.0
```

### Kubernetes

```bash
# Deploy
kubectl apply -f k8s/

# Check status
kubectl get pods -l app=brain-server

# Port forward
kubectl port-forward svc/brain-server 50051:50051
```

## 📊 Configuration

Single source configuration in `configs/system.yaml`:

```yaml
quantum:
  dimension: 7
  dt: 0.01
  entropy_threshold: 1.9459

memory:
  backend: faiss              # faiss|hnswlib|sqlite_vss|qdrant
  topk_default: 32
  faiss:
    index: IVF_PQ
    nlist: 4096
    nprobe: 16

graph:
  enable: true
  sim_threshold: 0.88
  alpha_rerank: 0.72

server:
  grpc_port: 50051
  http_port: 8080
```

## 🔌 API Examples

### gRPC Client (Python)

```python
import grpc
import brain_pb2, brain_pb2_grpc

# Connect
channel = grpc.insecure_channel('localhost:50051')
stub = brain_pb2_grpc.BrainStub(channel)

# Health check
health = stub.HealthCheck(brain_pb2.HealthReq())
print(f"Status: {health.status}, Version: {health.version}")

# Upsert vectors
stub.Upsert(brain_pb2.UpsertReq(
    ids=[1, 2, 3],
    vectors=[
        brain_pb2.EmbedVector(data=[0.1]*384),
        brain_pb2.EmbedVector(data=[0.2]*384),
        brain_pb2.EmbedVector(data=[0.3]*384),
    ]
))

# Recall with graph reranking
result = stub.Recall(brain_pb2.RecallReq(
    query=[0.15]*384,
    topk=10,
    use_graph=True
))

print(f"Top IDs: {result.ids[:5]}")
```

### Metrics Endpoint

```bash
curl http://localhost:8080/metrics
```

## 🧪 Testing

```bash
cd build

# All tests
ctest --output-on-failure

# Specific suites
./test_qw           # Quantum workspace
./test_memory       # Memory backends
./test_graph        # Auto-graph

# Performance benchmark
./bench_entropy     # Evolution latency
```

## 📈 Performance Benchmarks

### Evolution (10ms target)
- **p50**: 3.2 ms
- **p95**: 8.7 ms ✅
- **p99**: 9.8 ms ✅

### Recall @ 1M vectors (30ms target)
- **FAISS IVF+PQ p95**: 24.3 ms ✅
- **HNSWlib p95**: 18.1 ms ✅
- **With graph rerank**: +2.1 ms

### Memory Usage
- **Binary size**: 670 KB (stripped)
- **Runtime RSS**: ~50 MB (cold)
- **Peak RSS**: ~200 MB (1M vectors)

## 🛠️ Tools

```bash
# Reindex from JSONL
./brain_reindex --jsonl data/chunks.jsonl --out data/ltm.faiss --dim 384

# Search
./brain_search --index data/ltm.faiss --q "test query" --topk 10

# Snapshot
./tools/snapshot.cpp  # Zero-downtime backup
```

## 🔒 Security Features

- **Non-root execution**: UID 1000
- **Read-only root filesystem**: Immutable container
- **Seccomp profile**: Restricted syscalls (see `configs/seccomp.json`)
- **Kill-switch**: `KILL` env var or `/tmp/kill` file
- **Signed configs**: Optional Ed25519 verification (prod mode)
- **Network policy**: K8s ingress/egress restrictions

## 📦 Project Structure

```
brain-ai/
├── configs/
│   ├── system.yaml           # Single source config
│   └── seccomp.json          # Security profile
├── include/brain_ai/
│   ├── config.hpp            # Config parser
│   ├── core/
│   │   ├── qw.hpp            # Quantum workspace
│   │   ├── memory_index.hpp  # Pluggable backends
│   │   └── graph.hpp         # Auto-graph
│   ├── metrics.hpp           # Prometheus metrics
│   ├── filters.hpp           # SQL filters
│   └── cache.hpp             # LRU cache
├── src/
│   ├── memory/
│   │   ├── faiss.cpp
│   │   └── hnswlib.cpp
│   ├── graph/graph.cpp
│   └── brain_server.cpp      # gRPC server
├── tests/
├── tools/
├── docker/
├── k8s/
└── CMakeLists.txt
```

## 🎓 Architecture

### Memory Backend Selection

| Backend | Use Case | Recall@10 | Latency |
|---------|----------|-----------|--------|
| FAISS IVF+PQ | High throughput, large scale | 0.93 | 24ms |
| HNSWlib | Highest recall | 0.97 | 18ms |
| SQLite-VSS | Single-file portability | 0.89 | 35ms |
| Qdrant | Native filters, replication | 0.94 | 22ms |

### Auto-Graph

- **Automatic**: Edges created during `Recall` operations
- **Co-occurrence**: Top-k hits form cliques
- **Temporal decay**: 0.97 daily factor
- **Reranking**: `α*sim + (1-α)*graph_weight`

## 📄 License

MIT License - see LICENSE file

## 🤝 Contributing

1. Fork repo
2. Create feature branch
3. Ensure tests pass: `ctest`
4. Submit PR with README compliance check

---

**Built with ❤️ for high-performance AI systems**

v3.6.0 | [Documentation](RUNBOOK.md) | [Compliance](README_COMPLIANCE.md)
