#pragma once
#include <string>
#include <yaml-cpp/yaml.h>
#include <stdexcept>

namespace brain_ai {

struct QuantumConfig {
    int dimension = 7;
    double dt = 0.01;
    double decoherence_rate = 0.05;
    double entropy_threshold = 1.9459;
    double max_dwell_ms = 120.0;
    double collapse_rate_target_hz = 8.2;
    double trace_tolerance = 1e-9;
    double eigenvalue_floor = 1e-9;
};

struct EmbeddingConfig {
    int dim = 384;
    std::string model = "sentence-transformer";
    bool normalize = true;
};

struct MemoryConfig {
    std::string backend = "faiss";
    std::string path = "data/ltm.faiss";
    int topk_default = 32;
    int batch_upsert = 2048;
    
    struct Shard {
        bool enable = true;
        int shards = 4;
    } shard;
    
    struct FAISS {
        std::string index = "IVF_PQ";
        int nlist = 4096;
        int nprobe = 16;
        int pq_m = 32;
    } faiss;
    
    struct HNSW {
        int M = 32;
        int ef_construct = 200;
        int ef_search = 64;
    } hnsw;
};

struct SQLiteConfig {
    std::string db_path = "data/brain.db";
    bool mmap = true;
    bool wal = true;
};

struct QdrantConfig {
    std::string url = "http://127.0.0.1:6333";
    std::string collection = "brain_docs";
    int shard_count = 1;
};

struct FilterConfig {
    bool enable = true;
};

struct CacheConfig {
    bool enable = true;
    int size = 100000;
    int ttl_s = 300;
};

struct GraphConfig {
    bool enable = true;
    double sim_threshold = 0.88;
    int cooccur_k = 10;
    double reinforce_step = 0.05;
    double decay_daily = 0.97;
    int max_edges_per_node = 256;
    bool record_query_node = true;
    double alpha_rerank = 0.72;
    int fanout = 8;
    int temporal_boost_days = 14;
};

struct SecurityConfig {
    bool prod_mode = false;
    bool require_signed_config = false;
    std::string kill_env = "KILL";
    std::string kill_file = "/tmp/kill";
};

struct ServerConfig {
    int grpc_port = 50051;
    int http_port = 8080;
    int max_concurrency = 8;
    int rate_limit_qps = 50;
};

struct SystemConfig {
    QuantumConfig quantum;
    EmbeddingConfig embeddings;
    MemoryConfig memory;
    SQLiteConfig sqlite;
    QdrantConfig qdrant;
    FilterConfig filters;
    CacheConfig cache;
    GraphConfig graph;
    SecurityConfig security;
    ServerConfig server;
    
    static SystemConfig load(const std::string& path);
    void validate() const;
};

// Implementation
inline SystemConfig SystemConfig::load(const std::string& path) {
    YAML::Node config = YAML::LoadFile(path);
    SystemConfig cfg;
    
    if (config["quantum"]) {
        auto q = config["quantum"];
        cfg.quantum.dimension = q["dimension"].as<int>();
        cfg.quantum.dt = q["dt"].as<double>();
        cfg.quantum.decoherence_rate = q["decoherence_rate"].as<double>();
        cfg.quantum.entropy_threshold = q["entropy_threshold"].as<double>();
        cfg.quantum.max_dwell_ms = q["max_dwell_ms"].as<double>();
        cfg.quantum.collapse_rate_target_hz = q["collapse_rate_target_hz"].as<double>();
    }
    
    if (config["memory"]) {
        auto m = config["memory"];
        cfg.memory.backend = m["backend"].as<std::string>();
        cfg.memory.path = m["path"].as<std::string>();
        cfg.memory.topk_default = m["topk_default"].as<int>();
        
        if (m["faiss"]) {
            cfg.memory.faiss.nlist = m["faiss"]["nlist"].as<int>();
            cfg.memory.faiss.nprobe = m["faiss"]["nprobe"].as<int>();
            cfg.memory.faiss.pq_m = m["faiss"]["pq_m"].as<int>();
        }
    }
    
    if (config["graph"]) {
        auto g = config["graph"];
        cfg.graph.enable = g["enable"].as<bool>();
        cfg.graph.sim_threshold = g["sim_threshold"].as<double>();
        cfg.graph.alpha_rerank = g["alpha_rerank"].as<double>();
    }
    
    if (config["server"]) {
        cfg.server.grpc_port = config["server"]["grpc_port"].as<int>();
        cfg.server.http_port = config["server"]["http_port"].as<int>();
    }
    
    return cfg;
}

inline void SystemConfig::validate() const {
    if (quantum.dimension < 2 || quantum.dimension > 100) {
        throw std::runtime_error("Invalid quantum dimension");
    }
    if (memory.backend != "faiss" && memory.backend != "hnswlib" && 
        memory.backend != "sqlite_vss" && memory.backend != "qdrant") {
        throw std::runtime_error("Invalid memory backend");
    }
}

} // namespace brain_ai
