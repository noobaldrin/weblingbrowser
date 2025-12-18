#ifndef WEBLING_URL_DROPDOWN_HPP
#define WEBLING_URL_DROPDOWN_HPP

#include <memory>
#include <string>

#include <gtkmm/popover.h>
#include <gtkmm/listview.h>
#include <giomm/liststore.h>
#include <glibmm/property.h>

#include <sigc++/connection.h>

#include <SQLiteCpp/Database.h>

namespace Gtk {
    class Box;
    class Button;
    class Entry;
    class ScrolledWindow;
    class ListView;
    class ListItem;
    class SingleSelection;
}

namespace Gio {
    class Cancellable;
}

typedef struct GumboInternalNode GumboNode;
typedef struct _SoupSession SoupSession;

class UrlItem final : public Glib::Object {
public:
    UrlItem(const guint id,
            const guint item_pos,
            const Glib::ustring& name,
            const Glib::ustring& subdomain,
            const Glib::ustring& basedomain,
            const unsigned int favicon_id,
            const Glib::RefPtr<Gdk::Paintable>& favicon)
    : Glib::ObjectBase (typeid(UrlItem))
    , m_index (id)
    , m_item_position(item_pos)
    , m_name(*this, "url-name", name)
    , m_subdomain(*this, "sub-domain", subdomain)
    , m_base_domain(*this, "base-domain", basedomain)
    , m_full_domain(*this, "full-domain", "")
    , m_favicon_id(*this, "favicon-id", favicon_id)
    , m_favicon(*this, "favicon", favicon)
    , m_current_active(*this, "current-active", false) {
        update_full_url();

        m_subdomain.get_proxy().signal_changed().connect(
            [this] {
            update_full_url();
        });

        m_base_domain.get_proxy().signal_changed().connect(
            [this] {
            update_full_url();
        });
    }

    bool operator==(const UrlItem& rhs) const {
        return rhs.m_index == m_index;
    }

    Glib::PropertyProxy<Glib::ustring> property_urlname() {
        return m_name.get_proxy();
    }

    Glib::PropertyProxy<Glib::ustring> property_subdomain() {
        return m_subdomain.get_proxy();
    }

    Glib::PropertyProxy<Glib::ustring> property_basedomain() {
        return m_base_domain.get_proxy();
    }

    Glib::PropertyProxy<Glib::ustring> property_fulldomain() {
        return m_full_domain.get_proxy();
    }

    Glib::PropertyProxy<unsigned int> property_favicon_id() {
        return m_favicon_id.get_proxy();
    }

    Glib::PropertyProxy<Glib::RefPtr<Gdk::Paintable>> property_favicon() {
        return m_favicon.get_proxy();
    }

    Glib::PropertyProxy<bool> property_current_active() {
        return m_current_active.get_proxy();
    }

    guint get_idx() const {
        return m_index;
    }

private:
    void update_full_url() {
        if (m_subdomain.get_proxy().get_value().empty())
            m_full_domain.get_proxy().set_value(m_base_domain.get_value());
        else m_full_domain.get_proxy().set_value(m_subdomain.get_proxy() + "." + m_base_domain);
    }

    const guint m_index;
    guint m_item_position;
    Glib::Property<Glib::ustring> m_name;
    Glib::Property<Glib::ustring> m_subdomain;
    Glib::Property<Glib::ustring> m_base_domain;
    Glib::Property<Glib::ustring> m_full_domain;
    Glib::Property<unsigned int>  m_favicon_id;

    // Todo: stop storing the Paintable in this class
    Glib::Property<Glib::RefPtr<Gdk::Paintable>> m_favicon;
    Glib::Property<bool> m_current_active;
};

class UrlDropDown final : public Gtk::Popover {
public:
    explicit UrlDropDown(Gtk::Widget *menu_button);
    ~UrlDropDown() noexcept override;

    static void split_urlname(const std::string& url,
                              std::string& subdomain,
                              std::string& basedomain);

private:
    static bool validate_url(const Glib::ustring &url);
    static void send_msg_finished(GObject* source_object,
                                  GAsyncResult* res,
                                  gpointer user_data);
    void on_remove_button_pressed() const;
    void on_entry_text_changed(Gtk::Entry* entry);
    static bool on_debounce_timeout(Gtk::Entry* entry);

    static void setup_listitem_cb(const Glib::RefPtr<Gtk::ListItem>& list_item);
    static void bind_listitem_cb(const Glib::RefPtr<Gtk::ListItem>& list_item);
    void open_edit_url_dialog(const Glib::RefPtr<UrlItem>& item);
    void populate_listview(const Glib::RefPtr<Gio::ListStore<UrlItem>> &model) const;
    void activate_cb(guint position) const;

    static Glib::RefPtr<Gdk::Texture> decode_image_to_texture(const Glib::RefPtr<Gio::InputStream>& stream);
    static Glib::RefPtr<Gdk::Texture> decode_image_to_texture(const Glib::RefPtr<Glib::Bytes>& bytes);
    static Glib::RefPtr<Gdk::Texture> get_default_favicon(const gchar *hostname, const Glib::RefPtr<Gio::InputStream>& stream);
    static std::string parse_favicon_link_url(const std::string &str);
    static std::string resolve_url(const std::string& page_url);
    static std::vector<std::string> search_for_favicon_links(GumboNode *node);

    std::shared_ptr<SQLite::Database> m_url_db;
    sigc::connection debounce_conn;
    static Glib::RefPtr<Gio::Cancellable> m_url_cancellable;
    static SoupSession *m_soup_session;

    Glib::RefPtr<Gtk::SingleSelection> m_selection;
    Gtk::ListView *m_urls_listview;
    Gtk::MenuButton* m_menu_button;
};

#endif // WEBLING_URL_DROPDOWN_HPP
