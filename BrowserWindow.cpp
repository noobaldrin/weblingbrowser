#include <algorithm>
#include <memory>
#include <iostream>

#include <giomm/menu.h>
#include <giomm/simpleactiongroup.h>
#include <glibmm/miscutils.h>
#include <gtkmm/application.h>
#include <gtkmm/notebook.h>
#include <gtkmm/headerbar.h>
#include <gtkmm/box.h>
#include <gtkmm/button.h>
#include <gtkmm/image.h>
#include <gtkmm/cssprovider.h>
#include <gtkmm/popover.h>
#include <gtkmm/popovermenu.h>
#include <gtkmm/menubutton.h>
#include <gtkmm/stringlist.h>
#include <gtkmm/label.h>
#include <webkit/webkit.h>

#include "DBContext.h"
#include "FontSettings.h"
#include "Settings.h"
#include "UrlDropDown.h"
#include "BrowserWindow.h"

void on_webview_load_changed(WebKitWebView* web_view, WebKitLoadEvent load_event, gpointer user_data);

BrowserWindow::BrowserWindow()
    : m_current_session_str(Settings::getSessionSettingsInstance()->get_string("current-session-id"))
{
    set_title("Webling Browser");
    set_default_size(800, 600);

    auto css_provider = Gtk::CssProvider::create();
    css_provider->load_from_resource("/webling/res/style.css");
    Gtk::StyleContext::add_provider_for_display(Gdk::Display::get_default(),
                                                css_provider,
                                                GTK_STYLE_PROVIDER_PRIORITY_USER);

    set_decorated(true);
    auto header = Gtk::make_managed<Gtk::HeaderBar>();
    header->set_show_title_buttons(false);
    set_titlebar(*header);

    auto settings_menubtn = Gtk::make_managed<Gtk::MenuButton>();
    settings_menubtn->set_icon_name("open-menu-symbolic");
    auto settings_popovermenu = Gtk::make_managed<Gtk::PopoverMenu>();
    settings_popovermenu->set_has_arrow(false);

    auto root_menu = Gio::Menu::create();

    auto settings_menu = Gio::Menu::create();
    auto open_font_settings_action = Gio::SimpleAction::create("open-font-settings");
    open_font_settings_action->signal_activate().connect([this](const Glib::VariantBase&) {
        static std::shared_ptr<FontSettings> font_settings_window;
        if (!font_settings_window)
             font_settings_window = std::make_shared<FontSettings>(&m_webviews);

        font_settings_window->present();
    });

    auto clear_all_data_action = Gio::SimpleAction::create("clear-all-data");
    clear_all_data_action->signal_activate().connect([this](const Glib::VariantBase&) {
        webkit_website_data_manager_clear(m_datamanager, WEBKIT_WEBSITE_DATA_ALL, 0, nullptr,
                                          reinterpret_cast<GAsyncReadyCallback>(G_CALLBACK(+[](
                                                  GObject*,
                                                  GAsyncResult*,
                                                  gpointer) {
                                              std::cout << "All data cleared" << std::endl;
                                              })), nullptr);
    });

    auto actions_group = Gio::SimpleActionGroup::create();
    actions_group->add_action(open_font_settings_action);
    actions_group->add_action(clear_all_data_action);

    settings_menu->append("Change Font Settings", "main-menu.open-font-settings");
    settings_menu->append("Clear all data", "main-menu.clear-all-data");

    auto basic_actions_menu = Gio::Menu::create();
    auto basic_actions_group = Gio::SimpleActionGroup::create();
    auto exit_action = Gio::SimpleAction::create("quit");
    exit_action->signal_activate().connect([this](const Glib::VariantBase&) {
        close();
    });

    basic_actions_group->add_action(exit_action);
    basic_actions_menu->append("Exit", "win.quit");

    root_menu->append_section(settings_menu);
    root_menu->append_section(basic_actions_menu);
    signal_map().connect([this, actions_group, basic_actions_group]() {
            insert_action_group("main-menu", actions_group);
            insert_action_group("win", basic_actions_group);
    });

    settings_popovermenu->set_menu_model(root_menu);
    if (!m_window_font_settings)
        m_window_font_settings = std::make_unique<FontSettings>(&m_webviews);

    auto fontdialog_btn = Gtk::make_managed<Gtk::Button>("Change Fonts");
    fontdialog_btn->signal_clicked().connect([this]() {
        m_window_font_settings->set_modal(true);
        m_window_font_settings->set_hide_on_close(true);
        m_window_font_settings->show();
    });

    settings_menubtn->set_popover(*settings_popovermenu);

    m_notebook = Gtk::make_managed<Gtk::Notebook>();
    set_child(*m_notebook);

    auto userdatadir = Glib::get_user_data_dir();
    auto usercachedir = Glib::get_user_cache_dir();
    auto datadir = Glib::build_filename(userdatadir, "webling", nullptr);
    auto cachedir = Glib::build_filename(usercachedir, "webling", nullptr);
    auto cookiecache = Glib::build_filename(datadir, "cookies.db", nullptr);
    auto sessiondir = Glib::build_filename(datadir, "session", nullptr);
    auto url_db = Glib::build_filename(sessiondir, "urls.db", nullptr);

    m_networksession = webkit_network_session_new(datadir.c_str(), cachedir.c_str());
    webkit_network_session_set_persistent_credential_storage_enabled(m_networksession, TRUE);
    m_datamanager = webkit_network_session_get_website_data_manager(m_networksession);
    webkit_website_data_manager_set_favicons_enabled(m_datamanager, true);
    auto cookiemanager = webkit_network_session_get_cookie_manager(m_networksession);
    webkit_cookie_manager_set_persistent_storage(
        cookiemanager,
        cookiecache.c_str(),
        WEBKIT_COOKIE_PERSISTENT_STORAGE_SQLITE
    );

    auto action_widget_box = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL);

    auto add_tab_btn_cmbbox_box = Gtk::make_managed<Gtk::Box>();
    action_widget_box->append(*settings_menubtn);

    auto urlbutton = Gtk::make_managed<Gtk::MenuButton>();
    urlbutton->set_label("");
    auto urlpopover = Gtk::make_managed<UrlDropDown>(urlbutton);
    urlpopover->set_has_arrow(false);
    urlbutton->set_popover(*urlpopover);

    auto add_tab_btn = Gtk::make_managed<Gtk::Button>();
    add_tab_btn->set_icon_name("tab-new-symbolic");
    add_tab_btn->signal_clicked().connect(
        [this, urlbutton] {
            auto url_data = urlbutton->get_data("url");
            if (!url_data)
                return;

            auto url = static_cast<Glib::ustring*>(url_data);
            add_tab("https://" + *url);
    });

    add_tab_btn_cmbbox_box->append(*add_tab_btn);
    add_tab_btn_cmbbox_box->append(*urlbutton);
    action_widget_box->prepend(*add_tab_btn_cmbbox_box);

    m_notebook->set_action_widget(action_widget_box, Gtk::PackType::END);
    m_notebook->set_scrollable(true);
    m_notebook->signal_page_reordered().connect(
        [](Gtk::Widget* widget, const guint i) {
        widget->set_data("current-tab-index", GINT_TO_POINTER(i),
            [](void *data) {
            delete static_cast<int*>(data);
        });
    });

    auto db_context = DBContext::instance();
    m_sessions_db = db_context.get_sessions_db();

    auto session_ids = fetch_session_ids();
    open_last_opened_tabs(session_ids);

    if (m_notebook->get_n_pages() == 0)
        add_tab("about:blank");
}

BrowserWindow::~BrowserWindow()
{
    g_object_unref(m_networksession);
}

void BrowserWindow::add_tab(const Glib::ustring& url) {
    const auto webview = static_cast<WebKitWebView*>(g_object_new(WEBKIT_TYPE_WEB_VIEW,
                                                     "network-session",
                                                     m_networksession, nullptr));
    webkit_web_view_load_uri(webview, url.c_str());
    const auto view = Glib::wrap(GTK_WIDGET(webview)); // Convert to gtkmm
    view->set_expand(true);

    apply_common_settings(webview);

    const int current_page_index = m_notebook->append_page(*view, *create_tab_label(view));
    if (current_page_index < 0)
        return;

    m_notebook->set_tab_reorderable(*view, true);
    view->set_data("current_tab_index", GINT_TO_POINTER(current_page_index));
    m_notebook->set_current_page(current_page_index);
    if (current_page_index >= 0 && current_page_index <= static_cast<int>(m_webviews.size())) {
        m_webviews.insert(m_webviews.begin() + current_page_index, webview);
    }
}

Gtk::Widget* BrowserWindow::create_tab_label(Widget *tab_child) {
    auto box = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL);
    auto webview_c = WEBKIT_WEB_VIEW(tab_child->gobj());

    auto close_button = Gtk::make_managed<Gtk::Button>();
    close_button->add_css_class("tab-close-button");
    close_button->set_icon_name("window-close-symbolic");
    close_button->signal_clicked().connect([this, tab_child, webview_c] {
        m_notebook->remove_page(m_notebook->page_num(*tab_child));

        if (auto it = std::find(m_webviews.begin(), m_webviews.end(), webview_c); it != m_webviews.end())
            m_webviews.erase(it);
    });

    auto favicon_img = Gtk::make_managed<Gtk::Image>();
    g_signal_connect(webview_c, "notify::favicon", G_CALLBACK(+[](
        GObject* object,
        GParamSpec*,
        const gpointer user_data
    ) {
        auto* img = GTK_IMAGE(user_data);
        auto* view = WEBKIT_WEB_VIEW(object);
        auto favicontexture = webkit_web_view_get_favicon(view);
        if (!favicontexture) {
            std::cerr << "No favicon available" << std::endl;
            return;
        }

        gtk_image_set_from_paintable(img, GDK_PAINTABLE(favicontexture));
    }), favicon_img->gobj());

    auto label = Gtk::make_managed<Gtk::Label>();
    g_signal_connect(webview_c, "notify::title", G_CALLBACK(+[](
        GObject* object,
        GParamSpec*,
        const gpointer user_data) {
        auto tab_label = static_cast<Gtk::Label*>(user_data);
        tab_label->set_width_chars(10);
        tab_label->set_ellipsize(Pango::EllipsizeMode::END);
        tab_label->set_justify(Gtk::Justification::LEFT);
        if(const gchar* title = webkit_web_view_get_title(WEBKIT_WEB_VIEW(object))) {
            tab_label->set_text(title);
        } else tab_label->set_text("Untitled");

        if (const gchar* uri = webkit_web_view_get_uri(WEBKIT_WEB_VIEW(object))) {
            tab_label->set_tooltip_text(uri);
        }

    }), label);

    g_signal_connect(webview_c, "load-changed", G_CALLBACK(on_webview_load_changed), m_window_font_settings.get());

    box->prepend(*label);
    box->prepend(*favicon_img);
    box->append(*close_button);
    return box;
}

void BrowserWindow::clear_history() const {
    webkit_website_data_manager_clear(m_datamanager, WEBKIT_WEBSITE_DATA_ALL, 0, nullptr, clear_history_cb, nullptr);
}

void BrowserWindow::clear_history_cb(GObject* source, GAsyncResult* result, gpointer) {
    GError* error = nullptr;
    if (!webkit_website_data_manager_clear_finish(WEBKIT_WEBSITE_DATA_MANAGER(source), result, &error)) {
        g_warning("Failed to clear data: %s", error->message);
        g_clear_error(&error);
    } else
        g_message("Website data cleared.");
}

void BrowserWindow::open_font_dialog()
{
    //auto fontdialog = Gtk::FontDialog::create();

    //fontdialog->choose_family(this, );
}

std::vector<SessionEntry> BrowserWindow::fetch_session_ids() const
{
    std::vector<SessionEntry> session_ids;
    SQLite::Statement sessions_stmt(*m_sessions_db, R"SQL(
                                  SELECT *
                                  FROM "main"."sessions"
                                  WHERE session_id = ?
                                  )SQL");
    sessions_stmt.bind(1, m_current_session_str);
    while (sessions_stmt.executeStep()) {
        session_ids.emplace_back( SessionEntry {
                                  sessions_stmt.getColumn("session_id").getString(),
                                  sessions_stmt.getColumn("url").getString(),
                                  sessions_stmt.getColumn("tab_position").getInt() }
        );
    }

    return session_ids;
}

bool BrowserWindow::save_last_opened_tabs()
{
    // Store non-unique favicons
    SQLite::Statement remove_stmt(*m_sessions_db, R"SQL(
                                  DELETE FROM "main"."sessions"
                                  WHERE "session_id" = ?
                                  )SQL");
    remove_stmt.bind(1, m_current_session_str);
    remove_stmt.exec();

    SQLite::Statement insert_stmt(*m_sessions_db, R"SQL(
                                  INSERT INTO "main"."sessions"
                                  (session_id, url, tab_position)
                                  VALUES (?, ?, ?)
                                  )SQL");

    auto pages = m_notebook->get_pages();
    if (!pages)
        std::cerr << "!pages" << std::endl;

    std::vector<WebKitWebView*> webviews;
    for (int i = 0; i < pages->get_n_items(); ++i) {
        auto notebook_page = std::dynamic_pointer_cast<Gtk::NotebookPage>(pages->get_object(i));
        if (!notebook_page)
            std::cout << "!ob" << std::endl;

        webviews.emplace_back(WEBKIT_WEB_VIEW(notebook_page->get_child()->gobj()));
        if (!webviews.back()) {
            std::cerr << "!webviews" << std::endl;
            return false;
        }
    }

    int i = 0;
    int n_modified = 0;
    for (auto webview : webviews) {
        insert_stmt.reset();
        insert_stmt.bind(1, m_current_session_str);
        insert_stmt.bind(2, webkit_web_view_get_uri(webview));
        insert_stmt.bind(3, i);
        if (insert_stmt.exec())
            n_modified++;
        i++;
    }

    // Todo: implement reopened closed tabs
    while (m_notebook->get_n_pages() > 0)
        m_notebook->remove_page(0);

    return true;
}

void BrowserWindow::open_last_opened_tabs(std::vector<SessionEntry>& sessions)
{
    std::sort(sessions.begin(), sessions.end(),
        [](const SessionEntry& a, const SessionEntry& b)->bool {
            return a.position < b.position;
    });

    for (auto &session : sessions) {
        add_tab(session.url);
    }
}

void on_webview_load_changed(WebKitWebView*, const WebKitLoadEvent load_event, const gpointer user_data)
{
    auto fontsettingswindow = static_cast<FontSettings*>(user_data);

    if (load_event == WEBKIT_LOAD_COMMITTED) {
        fontsettingswindow->js_font_override(FontSettings::FontType::Default);
        fontsettingswindow->js_font_override(FontSettings::FontType::Monospace);
        fontsettingswindow->js_font_override(FontSettings::FontType::Serif);
        fontsettingswindow->js_font_override(FontSettings::FontType::SansSerif);
    }
}

void BrowserWindow::apply_common_settings(WebKitWebView *webview)
{
    auto wksettings = webkit_web_view_get_settings(webview);
    webkit_settings_set_enable_javascript(wksettings, TRUE);
    webkit_settings_set_enable_site_specific_quirks(wksettings, TRUE);
    webkit_settings_set_enable_webaudio(wksettings, TRUE);
    webkit_settings_set_enable_webgl(wksettings, TRUE);
    webkit_settings_set_allow_file_access_from_file_urls(wksettings, TRUE);
    webkit_settings_set_enable_mediasource(wksettings, TRUE);
    webkit_settings_set_user_agent(
        wksettings, "Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:140.0) Gecko/20100101 Firefox/140.0"
    );
}
