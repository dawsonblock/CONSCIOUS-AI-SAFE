#include "brain_ai/filters.hpp"
#include <sqlite3.h>
#include <stdexcept>
#include <sstream>

namespace brain_ai {

class MetadataFilter::Impl {
public:
    explicit Impl(const std::string& db_path) {
        if (sqlite3_open(db_path.c_str(), &db_) != SQLITE_OK) {
            throw std::runtime_error("Failed to open metadata database");
        }
        
        const char* schema = R"(
            CREATE TABLE IF NOT EXISTS meta (
                id INTEGER NOT NULL,
                key TEXT NOT NULL,
                val_num REAL,
                val_text TEXT,
                PRIMARY KEY (id, key)
            );
            CREATE INDEX IF NOT EXISTS idx_meta_key ON meta(key);
            CREATE INDEX IF NOT EXISTS idx_meta_num ON meta(val_num);
            CREATE INDEX IF NOT EXISTS idx_meta_text ON meta(val_text);
        )";
        
        char* err = nullptr;
        if (sqlite3_exec(db_, schema, nullptr, nullptr, &err) != SQLITE_OK) {
            std::string msg = err ? err : "Unknown error";
            sqlite3_free(err);
            throw std::runtime_error("Schema creation failed: " + msg);
        }
    }
    
    ~Impl() {
        if (db_) sqlite3_close(db_);
    }
    
    void set_metadata(int64_t id, const std::string& key, const std::string& val) {
        const char* sql = "INSERT OR REPLACE INTO meta (id, key, val_text) VALUES (?, ?, ?)";
        sqlite3_stmt* stmt;
        sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
        sqlite3_bind_int64(stmt, 1, id);
        sqlite3_bind_text(stmt, 2, key.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 3, val.c_str(), -1, SQLITE_STATIC);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
    }
    
    void set_metadata(int64_t id, const std::string& key, double val) {
        const char* sql = "INSERT OR REPLACE INTO meta (id, key, val_num) VALUES (?, ?, ?)";
        sqlite3_stmt* stmt;
        sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
        sqlite3_bind_int64(stmt, 1, id);
        sqlite3_bind_text(stmt, 2, key.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_double(stmt, 3, val);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
    }
    
    std::unordered_set<int64_t> filter_ids(const std::vector<FilterPredicate>& predicates) {
        if (predicates.empty()) {
            return {};
        }
        
        // Build WHERE clause
        std::ostringstream where;
        where << "SELECT DISTINCT id FROM meta WHERE ";
        
        for (size_t i = 0; i < predicates.size(); ++i) {
            if (i > 0) where << " AND ";
            
            const auto& pred = predicates[i];
            where << "(key = '" << pred.key << "' AND ";
            
            if (pred.op == "=") {
                if (!pred.value_text.empty()) {
                    where << "val_text = '" << pred.value_text << "'";
                } else {
                    where << "val_num = " << pred.value_num;
                }
            } else if (pred.op == ">") {
                where << "val_num > " << pred.value_num;
            } else if (pred.op == "<") {
                where << "val_num < " << pred.value_num;
            } else if (pred.op == ">=") {
                where << "val_num >= " << pred.value_num;
            } else if (pred.op == "<=") {
                where << "val_num <= " << pred.value_num;
            }
            
            where << ")";
        }
        
        // Execute query
        sqlite3_stmt* stmt;
        std::string sql = where.str();
        sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr);
        
        std::unordered_set<int64_t> result;
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            result.insert(sqlite3_column_int64(stmt, 0));
        }
        
        sqlite3_finalize(stmt);
        return result;
    }
    
private:
    sqlite3* db_ = nullptr;
};

MetadataFilter::MetadataFilter(const std::string& db_path)
    : impl_(std::make_unique<Impl>(db_path)) {}

MetadataFilter::~MetadataFilter() = default;

void MetadataFilter::set_metadata(int64_t id, const std::string& key, const std::string& val) {
    impl_->set_metadata(id, key, val);
}

void MetadataFilter::set_metadata(int64_t id, const std::string& key, double val) {
    impl_->set_metadata(id, key, val);
}

std::unordered_set<int64_t> MetadataFilter::filter_ids(
    const std::vector<FilterPredicate>& predicates) {
    return impl_->filter_ids(predicates);
}

} // namespace brain_ai
