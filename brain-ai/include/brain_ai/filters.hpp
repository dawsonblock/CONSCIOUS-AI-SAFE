#pragma once
#include <string>
#include <vector>
#include <unordered_set>
#include <memory>

namespace brain_ai {

struct FilterPredicate {
    std::string key;
    std::string op;  // "=", ">", "<", ">=", "<=", "IN"
    std::string value_text;
    double value_num = 0.0;
    std::vector<std::string> value_list;
};

class MetadataFilter {
public:
    explicit MetadataFilter(const std::string& db_path);
    ~MetadataFilter();
    
    // Store metadata
    void set_metadata(int64_t id, const std::string& key, const std::string& val);
    void set_metadata(int64_t id, const std::string& key, double val);
    
    // Query with filters
    std::unordered_set<int64_t> filter_ids(const std::vector<FilterPredicate>& predicates);
    
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace brain_ai
