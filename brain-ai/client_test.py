#!/usr/bin/env python3
"""Simple test client for Brain-AI gRPC server"""
import grpc
import brain_pb2
import brain_pb2_grpc

def test_health():
    print("\n🔍 Testing HealthCheck...")
    with grpc.insecure_channel('localhost:50051') as channel:
        stub = brain_pb2_grpc.BrainStub(channel)
        try:
            response = stub.HealthCheck(brain_pb2.HealthReq())
            print(f"✅ Status: {response.status}")
            print(f"   Version: {response.version}")
            print(f"   Uptime: {response.uptime_ms}ms")
            return True
        except Exception as e:
            print(f"❌ Error: {e}")
            return False

def test_step():
    print("\n🔍 Testing Step...")
    with grpc.insecure_channel('localhost:50051') as channel:
        stub = brain_pb2_grpc.BrainStub(channel)
        try:
            input_vec = [0.1] * 784
            response = stub.Step(brain_pb2.StepReq(input=input_vec, reward=0.0))
            print(f"✅ Entropy: {response.entropy:.4f}")
            print(f"   Total collapses: {response.total_collapses}")
            return True
        except Exception as e:
            print(f"❌ Error: {e}")
            return False

def test_upsert():
    print("\n🔍 Testing Upsert...")
    with grpc.insecure_channel('localhost:50051') as channel:
        stub = brain_pb2_grpc.BrainStub(channel)
        try:
            vectors = [brain_pb2.EmbedVector(data=[float(i)/100] * 384) for i in range(3)]
            response = stub.Upsert(brain_pb2.UpsertReq(ids=[1, 2, 3], vectors=vectors))
            print(f"✅ Upserted {response.count} vectors")
            return True
        except Exception as e:
            print(f"❌ Error: {e}")
            return False

def test_recall():
    print("\n🔍 Testing Recall...")
    with grpc.insecure_channel('localhost:50051') as channel:
        stub = brain_pb2_grpc.BrainStub(channel)
        try:
            query = [0.5] * 384
            response = stub.Recall(brain_pb2.RecallReq(query=query, topk=5, use_graph=True))
            print(f"✅ Retrieved {len(response.ids)} results")
            if response.ids:
                print(f"   Top IDs: {list(response.ids[:3])}")
            return True
        except Exception as e:
            print(f"❌ Error: {e}")
            return False

def test_stats():
    print("\n🔍 Testing GetStats...")
    with grpc.insecure_channel('localhost:50051') as channel:
        stub = brain_pb2_grpc.BrainStub(channel)
        try:
            response = stub.GetStats(brain_pb2.StatsReq())
            print(f"✅ Entropy: {response.entropy:.4f}")
            print(f"   Dimension: {response.dimension}")
            print(f"   Memory: {response.memory_stats.total_items} items")
            print(f"   Graph: {response.graph_stats.nodes} nodes, {response.graph_stats.edges} edges")
            return True
        except Exception as e:
            print(f"❌ Error: {e}")
            return False

if __name__ == '__main__':
    print("🧠 Brain-AI v3.6.0 - gRPC Client Test")
    print("=" * 50)
    
    results = [
        ('HealthCheck', test_health()),
        ('Step', test_step()),
        ('Upsert', test_upsert()),
        ('Recall', test_recall()),
        ('GetStats', test_stats()),
    ]
    
    print("\n" + "=" * 50)
    print("📊 Test Summary:")
    for name, passed in results:
        status = "✅ PASS" if passed else "❌ FAIL"
        print(f"   {name:<15} {status}")
    
    passed = sum(1 for _, p in results if p)
    print(f"\n   Total: {passed}/{len(results)} tests passed")
