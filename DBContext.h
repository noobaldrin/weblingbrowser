#ifndef WEBLING_DBCONTEXT_HPP
#define WEBLING_DBCONTEXT_HPP

#include <string>

namespace SQLite {
    class Database;
}

class DBContext {
public:
    ~DBContext();

    static DBContext& instance() {
        static DBContext ctx;
        return ctx;
    }

    std::shared_ptr<SQLite::Database> get_sessions_db() {
        return sessions_db_;
    }

    std::shared_ptr<SQLite::Database> get_urls_db() {
        return urls_db_;
    }

private:
    DBContext();

    const std::string default_sessions_db_path_;
    const std::string default_urls_db_path_;

    void fetch_configs_from_settings();
    static void create_directory_for_db_files();
    void create_database_files();

    std::shared_ptr<SQLite::Database> sessions_db_;
    std::shared_ptr<SQLite::Database> urls_db_;
};

#endif // WEBLING_DBCONTEXT_HPP