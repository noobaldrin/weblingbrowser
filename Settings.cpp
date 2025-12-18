#include <iostream>

#include "Settings.h"

#include <glibmm/datetime.h>

Settings::~Settings()
{
    saveSettings();
}

std::shared_ptr<Gio::Settings> Settings::m_font_settings = nullptr;
std::shared_ptr<Gio::Settings> Settings::m_sessions_settings = nullptr;
std::shared_ptr<Gio::Settings> Settings::m_db_settings = nullptr;

std::shared_ptr<Gio::Settings> Settings::getFontsSettingsInstance()
{
    try {
        if (!m_font_settings)
            m_font_settings = Gio::Settings::create("com.github.noobaldrin.webling.fonts");
    } catch (const Glib::Error &e) {
        std::cerr << "Failed to load font settings: " << e.what() << std::endl;
        return nullptr;
    }

    return m_font_settings;
}

std::shared_ptr<Gio::Settings> Settings::getSessionSettingsInstance()
{
    try {
        if (!m_sessions_settings)
            m_sessions_settings = Gio::Settings::create(
                                  "com.github.noobaldrin.webling.sessions");
        if (auto session_id = m_sessions_settings->get_string("current-session-id"); session_id.empty()) {
            auto now = Glib::DateTime::create_now_local();
            auto timestamp = now.format("%Y-%m-%dT%H:%M:%S");
            m_sessions_settings->set_string("current-session-id", timestamp);
            return m_sessions_settings;
        }
    } catch (const Glib::Error &e) {
        std::cerr << "Failed to load sessions settings: " << e.what() << std::endl;
        return nullptr;
    }

    return m_sessions_settings;
}

std::shared_ptr<Gio::Settings> Settings::getDBSettingsInstance()
{
    if (!m_db_settings)
        m_db_settings = Gio::Settings::create(
                        "com.github.noobaldrin.webling.databases");

    return m_db_settings;
}
void Settings::saveSettings()
{
}

void Settings::loadSettings()
{
}
