#ifndef WEBLING_BROWSER_WINDOW_HPP
#define WEBLING_BROWSER_WINDOW_HPP

#include <vector>

#include <gtkmm/window.h>

namespace Gtk {
    class Notebook;
    class CssProvider;
    class MenuButton;
}; // Gtk

namespace SQLite {
    class Database;
};

typedef struct _WebKitNetworkSession WebKitNetworkSession;
typedef struct _WebKitWebsiteDataManager WebKitWebsiteDataManager;
typedef struct _WebKitSettings WebKitSettings;
typedef struct _WebKitWebView WebKitWebView;

class FontSettings;

typedef struct SessionEntry_ {
    Glib::ustring session_id;
    Glib::ustring url;
    int position;
} SessionEntry;

class DBusConnection;

class BrowserWindow final : public Gtk::Window
{
public:
    BrowserWindow();
    ~BrowserWindow() override;

    void add_tab(const Glib::ustring& url);
    Gtk::Widget* create_tab_label(Gtk::Widget *tab_child);

    void clear_history() const;
    static void clear_history_cb(GObject* source, GAsyncResult* result, gpointer user_data);
    static void open_font_dialog();
    std::vector<SessionEntry> fetch_session_ids() const;

    void open_last_opened_tabs(std::vector<SessionEntry>& sessions);
    bool save_last_opened_tabs();
    void apply_common_settings(WebKitWebView* webview);

private:
    WebKitNetworkSession *m_networksession;
    WebKitWebsiteDataManager *m_datamanager;

    Gtk::Notebook *m_notebook;

    // TODO: remove coupling of this member
    std::unique_ptr<FontSettings> m_window_font_settings;
    std::vector<WebKitWebView*> m_webviews;
    Glib::RefPtr<SQLite::Database> m_sessions_db;
    Glib::ustring m_current_session_str;
};

#endif // WEBLING_BROWSER_WINDOW_HPP
