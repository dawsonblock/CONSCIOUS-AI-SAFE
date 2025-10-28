# Brain-AI v3.6.0 - Runbook

## 🚀 Deployment

### Local Development

```bash
# Build
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release
cmake --build build -j

# Run server
./build/brain_server configs/system.yaml
```

### Docker

```bash
# Build
docker build -t brain-ai:3.6.0 -f docker/Dockerfile .

# Run
docker run -d \
  -p 50051:50051 \
  -p 8080:8080 \
  -v $(pwd)/data:/app/data \
  brain-ai:3.6.0
```

### Kubernetes

```bash
# Apply manifests
kubectl apply -f k8s/

# Check pods
kubectl get pods -l app=brain-server

# Logs
kubectl logs -f deployment/brain-server

# Scale
kubectl scale deployment brain-server --replicas=5
```

## 🔧 Operations

### Reindexing

```bash
# Create new index from JSONL
./build/brain_reindex --jsonl data/chunks.jsonl --out data/ltm.faiss --dim 384

# Swap index (zero downtime)
./tools/snapshot --out snapshots/$(date +%F)
mv data/ltm.faiss data/ltm.faiss.old
mv data/ltm.faiss.new data/ltm.faiss
kill -HUP $(pidof brain_server)  # Reload
```

### Snapshots

```bash
# Manual snapshot
./tools/snapshot --out snapshots/backup-$(date +%s)

# Restore
cp snapshots/backup-123456789/* data/
kubectl rollout restart deployment/brain-server
```

### Monitoring

```bash
# Metrics
curl http://localhost:8080/metrics

# Health
grpc_health_probe -addr=localhost:50051

# Stats
grpcurl -plaintext localhost:50051 brain.Brain/GetStats
```

## 📊 Performance Tuning

### FAISS IVF+PQ

```yaml
memory:
  faiss:
    nlist: 4096      # More clusters = slower train, faster search
    nprobe: 16       # More probes = better recall, slower search
    pq_m: 32         # More subquants = better accuracy, more memory
```

### HNSWlib

```yaml
memory:
  hnsw:
    M: 32            # Higher M = better recall, more memory
    ef_construct: 200  # Higher = better index quality
    ef_search: 64     # Higher = better recall, slower search
```

### Graph

```yaml
graph:
  alpha_rerank: 0.72  # 0 = pure graph, 1 = pure similarity
  fanout: 8          # Neighbors to consider
  sim_threshold: 0.88  # Edge creation threshold
```

## 🔒 Security

### Production Mode

```yaml
security:
  prod_mode: true
  require_signed_config: true
```

### Kill-Switch

```bash
# Environment variable
export KILL=true

# File-based
touch /tmp/kill
```

### Seccomp

Applied automatically in Kubernetes via `seccompProfile: RuntimeDefault`

## 🐛 Troubleshooting

### Server won't start

```bash
# Check config
./build/brain_server configs/system.yaml --validate

# Check logs
journalctl -u brain-server -f

# Check ports
lsof -i :50051
```

### Slow queries

```bash
# Enable profiling
export GRPC_TRACE=all
export GRPC_VERBOSITY=DEBUG

# Check metrics
curl http://localhost:8080/metrics | grep recall_ms
```

### Memory issues

```bash
# Check current usage
ps aux | grep brain_server

# Adjust cache size
# Edit configs/system.yaml:
cache:
  size: 50000  # Reduce from 100K
```

## 📈 Scaling

### Horizontal (K8s)

```bash
# Manual scale
kubectl scale deployment brain-server --replicas=10

# Auto-scale (HPA already configured)
kubectl get hpa
```

### Vertical (Memory Backend)

```yaml
# Use sharding for >10M vectors
memory:
  shard:
    enable: true
    shards: 8  # Split across 8 shards
```

## 🔄 Updates

### Rolling Update (K8s)

```bash
# Build new image
docker build -t brain-ai:3.6.1 -f docker/Dockerfile .

# Push to registry
docker push myregistry.io/brain-ai:3.6.1

# Update deployment
kubectl set image deployment/brain-server brain-server=myregistry.io/brain-ai:3.6.1

# Monitor
kubectl rollout status deployment/brain-server
```

### Rollback

```bash
kubectl rollout undo deployment/brain-server
```

---

**For issues, see GitHub Issues or contact support**
