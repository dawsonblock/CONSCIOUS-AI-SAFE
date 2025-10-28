#include "brain_ai/core/memory_index.hpp"
#include "brain_ai/config.hpp"
#include <iostream>
#include <fstream>
#include <vector>
#include <sstream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

int main(int argc, char** argv) {
    std::string jsonl_path, output_path, backend = "hnswlib";
    int dim = 384;
    
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--jsonl" && i + 1 < argc) {
            jsonl_path = argv[++i];
        } else if (arg == "--out" && i + 1 < argc) {
            output_path = argv[++i];
        } else if (arg == "--dim" && i + 1 < argc) {
            dim = std::atoi(argv[++i]);
        } else if (arg == "--backend" && i + 1 < argc) {
            backend = argv[++i];
        }
    }
    
    if (jsonl_path.empty() || output_path.empty()) {
        std::cerr << "Usage: brain_reindex --jsonl <input.jsonl> --out <output.index> [--dim 384] [--backend hnswlib]\n";
        return 1;
    }
    
    std::cout << "🔄 Reindexing from: " << jsonl_path << "\n";
    std::cout << "   Backend: " << backend << "\n";
    std::cout << "   Dimension: " << dim << "\n";
    std::cout << "   Output: " << output_path << "\n\n";
    
    try {
        // Create index
        auto index = brain_ai::create_memory_index(backend, dim);
        
        // Read JSONL file
        std::ifstream file(jsonl_path);
        if (!file.is_open()) {
            std::cerr << "❌ Failed to open " << jsonl_path << "\n";
            return 1;
        }
        
        std::vector<std::vector<float>> vectors;
        std::vector<int64_t> ids;
        std::string line;
        int count = 0;
        
        while (std::getline(file, line)) {
            if (line.empty()) continue;
            
            try {
                json obj = json::parse(line);
                
                // Extract ID
                int64_t id = obj.value("id", count);
                
                // Extract embedding/vector
                std::vector<float> vec;
                if (obj.contains("embedding")) {
                    vec = obj["embedding"].get<std::vector<float>>();
                } else if (obj.contains("vector")) {
                    vec = obj["vector"].get<std::vector<float>>();
                } else {
                    std::cerr << "⚠️  Line " << count << " missing 'embedding' or 'vector' field\n";
                    continue;
                }
                
                if (vec.size() != static_cast<size_t>(dim)) {
                    std::cerr << "⚠️  Line " << count << " dimension mismatch: " 
                             << vec.size() << " != " << dim << "\n";
                    continue;
                }
                
                vectors.push_back(vec);
                ids.push_back(id);
                count++;
                
                if (count % 1000 == 0) {
                    std::cout << "   Processed " << count << " vectors...\n";
                }
                
            } catch (const std::exception& e) {
                std::cerr << "⚠️  Error parsing line " << count << ": " << e.what() << "\n";
            }
        }
        
        file.close();
        
        std::cout << "\n✅ Loaded " << count << " vectors\n";
        std::cout << "Adding to index...\n";
        
        // Add to index
        index->add(vectors, ids);
        
        std::cout << "✅ Added " << index->size() << " vectors\n";
        std::cout << "Saving index...\n";
        
        // Save index
        index->save(output_path);
        
        std::cout << "✅ Index saved to " << output_path << "\n";
        std::cout << "\n🎉 Reindexing complete!\n";
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "❌ Error: " << e.what() << "\n";
        return 1;
    }
}
