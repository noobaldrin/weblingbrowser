#include <iostream>
#include <memory>

#include <glibmm/miscutils.h>
#include <giomm/resource.h>
#include <giomm/settings.h>

#include "BrowserWindow.h"
#include "Settings.h"
#include "Webling.h"

Webling::Webling(const Glib::ustring& application_id,
                 const Gio::Application::Flags flags)
    : Gtk::Application(application_id, flags)
{
    const std::string  user_data_dir = Glib::get_user_data_dir();
    auto gres = Gio::Resource::create_from_file(user_data_dir + "/webling/resources/resource.gresource");
    gres->register_global();
    auto bytes = gres->lookup_data("/webling/res/introspection.xml");
    gsize size;
    auto data = bytes->get_data(size);
    m_introspection_xml = static_cast<const gchar*>(data);

    auto settings_path = user_data_dir + "/webling/schemas";

    g_setenv("GSETTINGS_SCHEMA_DIR", settings_path.c_str(), TRUE);
    m_fonts_settings = Settings::getFontsSettingsInstance();
}

//Webling::~Webling() = default;
Webling::~Webling()
{
    // to check if Gtk::application is actually exiting.
    std::cout << __PRETTY_FUNCTION__ << std::endl;
}

Glib::RefPtr<Webling> Webling::create(const Glib::ustring& application_id, Gio::Application::Flags flags)
{
    return Glib::make_refptr_for_instance<Webling>( new Webling(application_id, flags));
}

void Webling::on_startup() {
    Gtk::Application::on_startup();
    hold();

    auto introspection_info = Gio::DBus::NodeInfo::create_for_xml(m_introspection_xml);
    auto interface_info = introspection_info->lookup_interface("com.github.noobaldrin.webling");

    m_connection = Gio::DBus::Connection::get_sync(Gio::DBus::BusType::SESSION);
    dbusRegistrationId = m_connection->register_object(
        "/com/github/noobaldrin/webling",
        interface_info,
        //sigc::bind(sigc::ptr_fun(on_dbus_method_call), Glib::RefPtr<Webling>(this), m_window)
        sigc::mem_fun(*this, &Webling::on_dbus_method_call)
    );

    if (!m_window)
        m_window = Glib::make_refptr_for_instance<BrowserWindow>(new BrowserWindow());

    m_window->signal_close_request().connect([this]() {
        if (!m_window->save_last_opened_tabs())
            return true;

        release();
        return false;
    }, false);

    m_window->set_hide_on_close(false);
    add_window(*m_window);

    m_window->present();
}

void Webling::on_dbus_method_call(const Glib::RefPtr<Gio::DBus::Connection>&,
                         const Glib::ustring&,
                         const Glib::ustring&,
                         const Glib::ustring&,
                         const Glib::ustring& method_name,
                         const Glib::VariantContainerBase&,
                         const Glib::RefPtr<Gio::DBus::MethodInvocation>& invocation)
{
    if (method_name == "Toggle") {
        if (!m_window->is_active())
            m_window->set_visible(true);
        else m_window->set_visible(false);

    } else if (method_name == "Show") {
        m_window->set_visible(true);

    } else if (method_name == "Hide") {
        if (m_window->is_visible())
            m_window->set_visible(false);

    } else if (method_name == "Close") {
        m_window->close();
    } else if (method_name == "Debug") {
    }

    invocation->return_value(Glib::VariantContainerBase{});
}

void Webling::on_activate()
{
	std::cout << __PRETTY_FUNCTION__ << std::endl;
}

void Webling::on_shutdown()
{
    m_connection->unregister_object(dbusRegistrationId);
    Gtk::Application::on_shutdown();
}
