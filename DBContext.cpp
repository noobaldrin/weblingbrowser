#include <iostream>

#include <glibmm/fileutils.h>
#include <glibmm/miscutils.h>
#include <giomm/file.h>
#include <SQLiteCpp/Database.h>

#include "Settings.h"
#include "DBContext.h"

DBContext::DBContext()
    : default_sessions_db_path_(Glib::get_home_dir() + "/.local/share/webling/sessions/sessions.db")
    , default_urls_db_path_(Glib::get_home_dir() + "/.local/share/webling/sessions/urls.db")
{

    fetch_configs_from_settings();
    create_directory_for_db_files();
    create_database_files();
}

DBContext::~DBContext() = default;

void DBContext::fetch_configs_from_settings()
{
    auto db_settings = Settings::getDBSettingsInstance();
    auto settings_sessions_db_path = db_settings->get_string("sessions-db-path");
    auto settings_urls_db_path = db_settings->get_string("urls-db-path");
    if (settings_sessions_db_path.empty()) // Initialize the settings if no value
        db_settings->set_string("sessions-db-path", default_sessions_db_path_);
    if (settings_urls_db_path.empty())
        db_settings->set_string("urls-db-path", default_urls_db_path_);
}

void DBContext::create_directory_for_db_files()
{
    if (auto sessions_dir = Glib::get_home_dir() + "/.local/share/webling/sessions"; !Glib::file_test(sessions_dir, Glib::FileTest::EXISTS)) {
        if (auto dir = Gio::File::create_for_path(sessions_dir); dir->make_directory_with_parents())
            std::cout << sessions_dir << " has been created." << std::endl;
    }
}

void DBContext::create_database_files()
{
    auto db_settings = Settings::getDBSettingsInstance();
    auto sqliteFlags = SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE;
    sessions_db_ = std::make_shared<SQLite::Database>(
                   db_settings->get_string("sessions-db-path"),
                   sqliteFlags);
    urls_db_ = std::make_shared<SQLite::Database>(
               db_settings->get_string("urls-db-path"),
               sqliteFlags);
    urls_db_->exec("PRAGMA foreign_keys = ON;");

    if (!sessions_db_->tableExists("sessions"))
        sessions_db_->exec(R"SQL(
                CREATE TABLE "sessions" (
	            "session_id" TEXT NOT NULL,
	            "tab_position" INTEGER NOT NULL,
                "url" TEXT NOT NULL)
                )SQL");
    if (!urls_db_->tableExists("urls"))
        urls_db_->exec(R"SQL(
                    CREATE TABLE IF NOT EXISTS favicons (
                    id INTEGER PRIMARY KEY AUTOINCREMENT,
                    checksum TEXT UNIQUE NOT NULL,
                    favicon BLOB NOT NULL
                    );

                    CREATE TABLE IF NOT EXISTS urls (
                    id INTEGER PRIMARY KEY AUTOINCREMENT,
                    item_pos INTEGER NOT NULL,
                    name TEXT UNIQUE,
                    subdomain TEXT,
                    basedomain TEXT NOT NULL,
                    favicon_id INTEGER,
                    FOREIGN KEY(favicon_id) REFERENCES favicons(id));
                    )SQL");
}