#ifndef WEBLING_SETTINGS_HPP
#define WEBLING_SETTINGS_HPP

#include <memory>
#include <giomm/settings.h>

class Settings {
public:
    Settings() = delete;
    static std::shared_ptr<Gio::Settings> getFontsSettingsInstance();
    static std::shared_ptr<Gio::Settings> getSessionSettingsInstance();
    static std::shared_ptr<Gio::Settings> getDBSettingsInstance();


private:
    ~Settings();

    static void saveSettings();
    static void loadSettings();

    static std::shared_ptr<Gio::Settings> m_font_settings;
    static std::shared_ptr<Gio::Settings> m_sessions_settings;
    static std::shared_ptr<Gio::Settings> m_db_settings;
};

#endif // WEBLING_SETTINGS_HPP
