#!/usr/bin/env python3
"""
Simple test client for Brain-AI gRPC server
"""
import sys
import grpc
sys.path.append('/app/brain-ai/build/proto_gen')

# Generate proto files if needed
import subprocess
subprocess.run([
    'protoc',
    '--python_out=/app/brain-ai/build/proto_gen',
    '--grpc_python_out=/app/brain-ai/build/proto_gen', 
    '--plugin=protoc-gen-grpc_python=/usr/local/bin/grpc_python_plugin',
    '-I/app/brain-ai/proto',
    '/app/brain-ai/proto/brain.proto'
], capture_output=True)

try:
    import brain_pb2
    import brain_pb2_grpc
except ImportError:
    print("❌ Failed to import protobuf modules")
    sys.exit(1)

def test_health():
    """Test health check endpoint"""
    print("\\n🔍 Testing HealthCheck...")
    with grpc.insecure_channel('localhost:50051') as channel:
        stub = brain_pb2_grpc.BrainStub(channel)
        try:
            response = stub.HealthCheck(brain_pb2.HealthReq())
            print(f"✅ Status: {response.status}")
            print(f"   Version: {response.version}")
            print(f"   Uptime: {response.uptime_ms}ms")
            return True
        except grpc.RpcError as e:
            print(f"❌ Error: {e.code()} - {e.details()}")
            return False

def test_step():
    """Test quantum evolution step"""
    print("\\n🔍 Testing Step...")
    with grpc.insecure_channel('localhost:50051') as channel:
        stub = brain_pb2_grpc.BrainStub(channel)
        try:
            # Random input vector (784D)
            input_vec = [0.1] * 784
            response = stub.Step(brain_pb2.StepReq(input=input_vec, reward=0.0))
            print(f"✅ Entropy: {response.entropy:.4f}")
            print(f"   Total collapses: {response.total_collapses}")
            return True
        except grpc.RpcError as e:
            print(f"❌ Error: {e.code()} - {e.details()}")
            return False

def test_upsert():
    """Test upserting vectors"""
    print("\\n🔍 Testing Upsert...")
    with grpc.insecure_channel('localhost:50051') as channel:
        stub = brain_pb2_grpc.BrainStub(channel)
        try:
            # Add 3 test vectors
            vectors = [
                brain_pb2.EmbedVector(data=[float(i)/100] * 384)
                for i in range(3)
            ]
            response = stub.Upsert(brain_pb2.UpsertReq(
                ids=[1, 2, 3],
                vectors=vectors
            ))
            print(f"✅ Upserted {response.count} vectors")
            return True
        except grpc.RpcError as e:
            print(f"❌ Error: {e.code()} - {e.details()}")
            return False

def test_recall():
    """Test vector recall"""
    print("\\n🔍 Testing Recall...")
    with grpc.insecure_channel('localhost:50051') as channel:
        stub = brain_pb2_grpc.BrainStub(channel)
        try:
            query = [0.5] * 384
            response = stub.Recall(brain_pb2.RecallReq(
                query=query,
                topk=5,
                use_graph=True
            ))
            print(f"✅ Retrieved {len(response.ids)} results")
            if response.ids:
                print(f"   Top IDs: {list(response.ids[:3])}")
                print(f"   Distances: {[f'{d:.3f}' for d in response.distances[:3]]}")
            return True
        except grpc.RpcError as e:
            print(f"❌ Error: {e.code()} - {e.details()}")
            return False

def test_stats():
    """Test stats endpoint"""
    print("\\n🔍 Testing GetStats...")
    with grpc.insecure_channel('localhost:50051') as channel:
        stub = brain_pb2_grpc.BrainStub(channel)
        try:
            response = stub.GetStats(brain_pb2.StatsReq())
            print(f"✅ Entropy: {response.entropy:.4f}")
            print(f"   Dimension: {response.dimension}")
            print(f"   Memory items: {response.memory_stats.total_items}")
            print(f"   Graph nodes: {response.graph_stats.nodes}")
            print(f"   Graph edges: {response.graph_stats.edges}")
            return True
        except grpc.RpcError as e:
            print(f"❌ Error: {e.code()} - {e.details()}")
            return False

if __name__ == '__main__':
    print("🧠 Brain-AI v3.6.0 - gRPC Client Test")
    print("=" * 50)
    
    results = []
    results.append(('HealthCheck', test_health()))
    results.append(('Step', test_step()))
    results.append(('Upsert', test_upsert()))
    results.append(('Recall', test_recall()))
    results.append(('GetStats', test_stats()))
    
    print("\\n" + "=" * 50)
    print("📊 Test Summary:")
    for name, passed in results:
        status = "✅ PASS" if passed else "❌ FAIL"
        print(f"   {name:<15} {status}")
    
    total = len(results)
    passed = sum(1 for _, p in results if p)
    print(f"\\n   Total: {passed}/{total} tests passed")
    
    sys.exit(0 if passed == total else 1)
