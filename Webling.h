#ifndef WEBLING_HPP
#define WEBLING_HPP

#include <gtkmm/application.h>

namespace Gio {
    class Settings;
}

class BrowserWindow;

class Webling final : public Gtk::Application {
public:
    Webling() = delete;
    explicit Webling(const Glib::ustring& application_id = {},
                Gio::Application::Flags flags = Gio::Application::Flags::NONE);
    //~Webling() override;
    ~Webling();

    static Glib::RefPtr<Webling> create(const Glib::ustring& application_id, Gio::Application::Flags flags);

private:
    void on_startup() override;
    void on_activate() override;
    void on_shutdown() override;

    Glib::ustring m_introspection_xml;
    std::shared_ptr<Gio::Settings> m_fonts_settings;
    std::shared_ptr<Gio::Settings> m_sessions_settings;
    Glib::RefPtr<BrowserWindow> m_window;

    Glib::RefPtr<Gio::DBus::Connection> m_connection;
    guint dbusRegistrationId;
    void on_dbus_method_call(const Glib::RefPtr<Gio::DBus::Connection>&,
                             const Glib::ustring&,
                             const Glib::ustring&,
                             const Glib::ustring&,
                             const Glib::ustring& method_name,
                             const Glib::VariantContainerBase&,
                             const Glib::RefPtr<Gio::DBus::MethodInvocation>& invocation);
};


#endif // WEBLING_HPP
