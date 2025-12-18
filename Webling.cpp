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

Webling::~Webling() = default;

Glib::RefPtr<Webling> Webling::create(const Glib::ustring& application_id, Gio::Application::Flags flags)
{
    return std::make_shared<Webling>( application_id, flags );
}

void Webling::on_startup() {
    Gtk::Application::on_startup();

    auto introspection_info = Gio::DBus::NodeInfo::create_for_xml(m_introspection_xml);
    auto interface_info = introspection_info->lookup_interface("com.github.noobaldrin.webling");
    auto connection = Gio::DBus::Connection::get_sync(Gio::DBus::BusType::SESSION);

    connection->register_object(
        "/com/github/noobaldrin/webling",
        interface_info,
        sigc::mem_fun(*this, &Webling::on_dbus_method_call)
    );
}

#include <iostream>
void Webling::on_dbus_method_call(const Glib::RefPtr<Gio::DBus::Connection>&,
                         const Glib::ustring&,
                         const Glib::ustring&,
                         const Glib::ustring&,
                         const Glib::ustring& method_name,
                         const Glib::VariantContainerBase&,
                         const Glib::RefPtr<Gio::DBus::MethodInvocation>& invocation)
{
    static int i = 0;
    if (method_name == "Toggle") {
        if (!m_window->is_active())
            m_window->show();
        else m_window->hide();

        invocation->return_value(Glib::VariantContainerBase{});
    } else if (method_name == "Show") {
        m_window->show();

        invocation->return_value(Glib::VariantContainerBase{});
    } else if (method_name == "Hide") {
        if (m_window->is_visible())
            m_window->hide();
        invocation->return_value(Glib::VariantContainerBase{});
    }
    else if (method_name == "Close") {
        release();
        m_window->close();
    } else if (method_name == "Debug") {
        std::cout << i << ":IS_ACTIVE: " << m_window->is_active() << std::endl;
        std::cout << i << ":IS_VISIBLE: " << m_window->is_visible() << std::endl;
        std::cout << i << ":HAS_FOCUS: " << m_window->has_focus() << std::endl;
        i++;
    }
}

void Webling::on_activate()
{
    m_window = std::make_shared<BrowserWindow>();
    m_window->set_hide_on_close(false);
    add_window(*m_window);

    m_window->present();
    hold();
}
