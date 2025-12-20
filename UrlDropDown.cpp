#include <iostream>
#include <cstring>
#include <sstream>

#include <glibmm/main.h>
#include <glibmm/miscutils.h>
#include <glibmm/uriutils.h>
#include <glibmm/bytes.h>
#include <glibmm/refptr.h>
#include <glibmm/binding.h>
#include <glibmm/checksum.h>
#include <giomm/file.h>
#include <giomm/cancellable.h>
#include <giomm/liststore.h>
#include <giomm/inputstream.h>

#include <gdkmm/texture.h>
#include <gtkmm/box.h>
#include <gtkmm/button.h>
#include <gtkmm/entry.h>
#include <gtkmm/separator.h>
#include <gtkmm/scrolledwindow.h>
#include <gtkmm/listview.h>
#include <gtkmm/listitem.h>
#include <gtkmm/signallistitemfactory.h>
#include <gtkmm/singleselection.h>
#include <gtkmm/label.h>
#include <gtkmm/menubutton.h>
#include <gtkmm/window.h>
#include <gtkmm/filedialog.h>
#include <gtkmm/filefilter.h>
#include <gtkmm/error.h>
#include <libsoup/soup-session.h>
#include <libsoup/soup-message.h>
#include <libsoup/soup-types.h>
#include <gumbo.h>
#include <libpsl.h>
#include <glycin-gtk4.h>

#include "DBContext.h"
#include "UrlDropDown.h"

SoupSession* UrlDropDown::m_soup_session = nullptr;
Glib::RefPtr<Gio::Cancellable> UrlDropDown::m_url_cancellable = nullptr;

UrlDropDown::UrlDropDown(Gtk::Widget *menu_button)
{
    m_url_db = DBContext::instance().get_urls_db();
    m_soup_session = soup_session_new();
    soup_session_set_user_agent(m_soup_session,
    "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/117.0.0.0 Safari/537.36");

    m_menu_button = dynamic_cast<Gtk::MenuButton*>(menu_button);

    auto mainbox = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL);
    auto listview_scroll = Gtk::make_managed<Gtk::ScrolledWindow>();
    listview_scroll->set_max_content_height(1000);
    listview_scroll->set_min_content_height(100);
    auto factory = Gtk::SignalListItemFactory::create();
    factory->signal_setup().connect(sigc::ptr_fun(setup_listitem_cb));
    factory->signal_bind().connect(sigc::ptr_fun(bind_listitem_cb));

    const auto model = Gio::ListStore<UrlItem>::create();

    m_selection = Gtk::SingleSelection::create(model);
    m_selection->signal_items_changed().connect(
        [this](const guint position, guint, guint) {
        if (position == 0)
            m_urls_listview->activate_action("list.activate-item", Glib::Variant<guint>::create(position));
    });

    if (m_selection->get_n_items() == 0)
        m_menu_button->set_label("Click here to add your URL");

    m_urls_listview = Gtk::make_managed<Gtk::ListView>(m_selection, factory);
    m_urls_listview->add_css_class("url-listview");
    m_urls_listview->get_style_context()->add_class("navigation-sidebar");

    m_urls_listview->signal_activate().connect(
        sigc::mem_fun(*this, &UrlDropDown::activate_cb));

    listview_scroll->set_child(*m_urls_listview);
    mainbox->append(*listview_scroll);
    mainbox->append(*Gtk::make_managed<Gtk::Separator>(Gtk::Orientation::HORIZONTAL));

    auto buttonsbox = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL);
    auto add_button = Gtk::make_managed<Gtk::Button>();
    add_button->set_tooltip_text("Add new URL.");
    add_button->set_icon_name("list-add-symbolic");
    add_button->signal_clicked().connect(
        [this] {
        open_edit_url_dialog(nullptr);
    });

    auto remove_button = Gtk::make_managed<Gtk::Button>();
    remove_button->set_tooltip_text("Remove currently selected URL.");
    remove_button->set_icon_name("edit-delete-symbolic");
    remove_button->signal_clicked().connect(sigc::mem_fun(*this, &UrlDropDown::on_remove_button_pressed));

    auto edit_button = Gtk::make_managed<Gtk::Button>();
    edit_button->set_tooltip_text("Edit current selected URL.");
    edit_button->signal_clicked().connect(
        [this] {
        open_edit_url_dialog(std::dynamic_pointer_cast<UrlItem>(m_selection->get_selected_item()));
    });
    edit_button->set_icon_name("document-edit-symbolic");

    buttonsbox->set_halign(Gtk::Align::END);
    buttonsbox->append(*add_button);
    buttonsbox->append(*edit_button);
    buttonsbox->append(*remove_button);

    mainbox->append(*buttonsbox);
    mainbox->set_expand(true);
    set_child(*mainbox);
    set_autohide(false);

    populate_listview(model);

    m_urls_listview->activate_action("list.activate-item", Glib::Variant<guint>::create(0));
}

UrlDropDown::~UrlDropDown() noexcept = default;

void UrlDropDown::open_edit_url_dialog(const Glib::RefPtr<UrlItem> &item)
{
    auto dialog = Gtk::make_managed<Gtk::Window>();
    dialog->set_size_request(300, 300);
    auto parent = m_urls_listview->get_ancestor(GTK_TYPE_WINDOW);
    dialog->set_transient_for(*dynamic_cast<Gtk::Window*>(parent));
    dialog->set_modal(true);

    auto mainbox = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL);
    mainbox->add_css_class("dialog-box");
    auto favicon_box = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL);
    favicon_box->set_valign(Gtk::Align::CENTER);
    favicon_box->set_halign(Gtk::Align::CENTER);
    favicon_box->add_css_class("linked");
    auto image_for_picture_button = Gtk::make_managed<Gtk::Image>();
    image_for_picture_button->set_pixel_size(128);
    auto texture_for_picture_button = Gdk::Texture::create_from_resource("/webling/res/icons/image-missing.svg");
    image_for_picture_button->set(texture_for_picture_button);
    auto picture_button = Gtk::make_managed<Gtk::Button>();
    picture_button->set_child(*image_for_picture_button);
    picture_button->set_size_request(100, 100);
    picture_button->signal_clicked().connect([image_for_picture_button]() {
        auto filedialog = Gtk::FileDialog::create();
        filedialog->set_title("Select image/icon");
        filedialog->set_modal(true);

        auto filefilter = Gtk::FileFilter::create();
        filefilter->add_mime_type("image/*");
        auto filefilterlistmodel = Gio::ListStore<Gtk::FileFilter>::create();
        filefilterlistmodel->append(filefilter);
        filedialog->set_filters(filefilterlistmodel);

        filedialog->open([self = filedialog, image_for_picture_button](const Glib::RefPtr<Gio::AsyncResult>& result) {
            auto file = self->open_finish(result);
            auto stream = file->read();
            image_for_picture_button->set(decode_image_to_texture(file->read()));
        });
    });

    auto set_default_favicon_button = Gtk::make_managed<Gtk::Button>();
    set_default_favicon_button->set_tooltip_text("Get default favicon");
    set_default_favicon_button->set_size_request(1, 100);
    set_default_favicon_button->set_image_from_icon_name("view-refresh-symbolic", Gtk::IconSize::NORMAL);

    favicon_box->append(*picture_button);
    favicon_box->append(*set_default_favicon_button);

    auto dialog_name_entry = Gtk::make_managed<Gtk::Entry>();
    dialog_name_entry->set_placeholder_text("Enter name (ex. Gemini)");

    auto dialog_url_entry = Gtk::make_managed<Gtk::Entry>();
    dialog_url_entry->set_placeholder_text("Enter URL (ex. gemini.google.com)");
    set_default_favicon_button->signal_clicked().connect([dialog_url_entry, image_for_picture_button]() {
        if (!dialog_url_entry->get_data("default-favicon"))
            return;

        auto raw_ptr = dialog_url_entry->get_data("default-favicon");
        auto texture_ptr = static_cast<Glib::RefPtr<Gdk::Texture>*>(raw_ptr);
        Glib::RefPtr<Gdk::Texture> texture = *texture_ptr;
        image_for_picture_button->set(texture);
    });

    dialog_url_entry->signal_changed().connect(
        sigc::bind(
            sigc::mem_fun(*this, &UrlDropDown::on_entry_text_changed),
            dialog_url_entry)
    );

    auto exit_button = Gtk::make_managed<Gtk::Button>("Exit");
    exit_button->signal_clicked().connect([dialog]() {
        dialog->close();
    });

    auto response_button_box = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL);
    auto confirm_button = Gtk::make_managed<Gtk::Button>("Add");
    auto confirm_button_signal = confirm_button->signal_clicked().connect(
        [this, dialog, dialog_name_entry, dialog_url_entry, image_for_picture_button] {
        auto image_paintable = std::dynamic_pointer_cast<Gdk::Texture>(image_for_picture_button->get_paintable());
        auto png_bytes = image_paintable->save_to_png_bytes();
        gsize data_size;
        auto data = png_bytes->get_data(data_size);
        auto forchecksum = static_cast<const guchar*>(data);

        if (dialog_name_entry->get_text().empty() || dialog_url_entry->get_text().empty())
            return;

        SQLite::Statement stmt_find_existing_url_name(*m_url_db, R"SQL(
                                                        SELECT name, basedomain, subdomain
                                                        FROM "main"."urls"
                                                        WHERE name = ? OR (basedomain = ? AND subdomain = ?)
                                                        )SQL");
        stmt_find_existing_url_name.bind(1, dialog_name_entry->get_text());
        std::string basedomain; std::string subdomain;
        split_urlname(dialog_url_entry->get_text(), subdomain, basedomain);
        stmt_find_existing_url_name.bind(2, basedomain);
        stmt_find_existing_url_name.bind(3, subdomain);
        
        if (stmt_find_existing_url_name.executeStep()) {
            auto dialogmsg = Gtk::make_managed<Gtk::Window>();
            dialogmsg->set_transient_for(*dialog);
            dialogmsg->set_modal();
            auto queriedName = stmt_find_existing_url_name.getColumn(0).getString();
            auto queriedbasedomain = stmt_find_existing_url_name.getColumn(1).getString();
            auto queriedsubdomain = stmt_find_existing_url_name.getColumn(2).getString();

            Glib::ustring message;
            if (dialog_name_entry->get_text() == queriedName) {
                message = dialog_name_entry->get_text();
            } else if (dialog_url_entry->get_text() == Glib::ustring(queriedsubdomain + "." + queriedbasedomain)) {
                message = dialog_url_entry->get_text();
            } else if (dialog_url_entry->get_text() == queriedbasedomain) {
                message = dialog_url_entry->get_text();
            }

            auto messagelbl = Gtk::make_managed<Gtk::Label>(message + " already exists");
            messagelbl->set_expand(true);
            auto messageokbtn = Gtk::make_managed<Gtk::Button>("Ok");
            messageokbtn->set_expand(false);
            messageokbtn->signal_clicked().connect([dialogmsg]() {
                dialogmsg->close();
            });
            auto dialogbox = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL);
            dialogbox->add_css_class("dialog-box");
            dialogbox->append(*messagelbl);
            dialogbox->append(*messageokbtn);
            dialogmsg->set_child(*dialogbox);
            dialogmsg->present();
            return;
        }

        SQLite::Statement stmt_insert_favicon(*m_url_db, R"SQL(
                                                        INSERT OR IGNORE INTO "main"."favicons" (checksum, favicon)
                                                        VALUES (?, ?);
                                                        )SQL");
        auto checksumstr = Glib::Checksum::compute_checksum(Glib::Checksum::Type::SHA256, forchecksum, data_size);
        stmt_insert_favicon.bind(1, checksumstr);
        stmt_insert_favicon.bind(2, data, data_size);

        int favicon_id = 0;
        if (stmt_insert_favicon.exec() == 0) {
        // Get the existing favicon.id in case the last insert wasn't successful due to UNIQUE constraint
            if ((favicon_id = m_url_db->getLastInsertRowid()) == 0) {
                SQLite::Statement stmt_get_existing_favicon(*m_url_db, R"SQL(
                                                                SELECT id
                                                                FROM favicons
                                                                WHERE checksum = ?
                                                                )SQL");
                stmt_get_existing_favicon.bind(1, checksumstr);
                if (stmt_get_existing_favicon.executeStep())
                    favicon_id = stmt_get_existing_favicon.getColumn("id").getInt();
            }
        } else favicon_id = m_url_db->getLastInsertRowid();

        SQLite::Statement stmt_insert_db(*m_url_db, R"SQL(
                                        INSERT INTO "main"."urls"
                                        (item_pos, name, subdomain, basedomain, favicon_id)
                                        VALUES(?, ?, ?, ?, ?)
                                        )SQL");
        unsigned int item_pos = m_selection->get_n_items() + 1;
        stmt_insert_db.bind(1, item_pos);
        stmt_insert_db.bind(2, dialog_name_entry->get_text());

        stmt_insert_db.bind(3, subdomain);
        stmt_insert_db.bind(4, basedomain);
        stmt_insert_db.bind(5, favicon_id);
        if (auto nchange = stmt_insert_db.exec())
            std::cout << "Inserted " << nchange << " items." << std::endl;

        auto model = std::dynamic_pointer_cast<Gio::ListStore<UrlItem>>(m_selection->get_model());

        SQLite::Statement stmt_get_id(*m_url_db, R"SQL(SELECT id
                                                  FROM "main"."urls"
                                                  WHERE subdomain = ? AND basedomain = ?)SQL");
        stmt_get_id.bind(1, subdomain);
        stmt_get_id.bind(2, basedomain);
        if (auto id = stmt_get_id.executeStep()) {
            id = stmt_get_id.getColumn("id").getInt();
            model->append(Glib::make_refptr_for_instance<UrlItem>(
            new UrlItem(id,
            item_pos,
            dialog_name_entry->get_text(),
            subdomain,
            basedomain,
            favicon_id,
            image_paintable)));

            dialog->close();
        }
        });

    if (item) {
        if (confirm_button_signal.connected()) {
            confirm_button_signal.disconnect();
            confirm_button->signal_clicked().connect(
                [this, dialog_url_entry, dialog_name_entry, image_for_picture_button, item] {
                auto favicon = std::dynamic_pointer_cast<Gdk::Texture>(image_for_picture_button->get_paintable());
                auto bytes = favicon->save_to_png_bytes();
                gsize data_size;
                auto data = static_cast<const guchar*>(bytes->get_data(data_size));
                Glib::Checksum checksum;

                // Check if favicon checksum already exists
                auto checksumstr = checksum.compute_checksum(Glib::Checksum::Type::SHA256, data, data_size);
                SQLite::Statement stmt_get_row(*m_url_db, R"SQL(
                    SELECT id
                    FROM "main"."favicons"
                    WHERE checksum = ?
                )SQL");
                stmt_get_row.bind(1, checksumstr);
                auto favicon_id = item->property_favicon_id().get_value();
                if (stmt_get_row.executeStep()) {
                    std::cout << "Duplicate favicon found: favicon id " << favicon_id << std::endl;
                    favicon_id = stmt_get_row.getColumn("id").getInt();
                } else {
                    SQLite::Statement stmt_insert_favicon(*m_url_db, R"SQL(
                        INSERT INTO "main"."favicons"
                        (checksum, favicon)
                        VALUES(?, ?)
                    )SQL");
                    stmt_insert_favicon.bind(1, checksumstr);
                    stmt_insert_favicon.bind(2, data, data_size);
                    stmt_insert_favicon.exec();
                    favicon_id = m_url_db->getLastInsertRowid();
                }

                SQLite::Statement stmt_update_db(*m_url_db, R"SQL(
                      UPDATE "main"."urls"
                      SET name = ?,
                          subdomain = ?,
                          basedomain = ?,
                          favicon_id = ?
                      WHERE urls.id = ?
                )SQL");

                std::string subdomain;
                std::string basedomain;
                split_urlname(dialog_url_entry->get_text(), subdomain, basedomain);

                stmt_update_db.bind(1, dialog_name_entry->get_text());
                stmt_update_db.bind(2, subdomain);
                stmt_update_db.bind(3, basedomain);
                auto image_paintable = std::dynamic_pointer_cast<Gdk::Texture>(
                                       image_for_picture_button->get_paintable());
                auto png_bytes = image_paintable->save_to_png_bytes();

                stmt_update_db.bind(4, favicon_id);
                stmt_update_db.bind(5, item->get_idx());

                if (auto nchange = stmt_update_db.exec())
                    std::cout << "Updated: " << nchange << " changes" << std::endl;

                item->property_urlname().set_value(dialog_name_entry->get_text());
                item->property_subdomain().set_value(subdomain);
                item->property_basedomain().set_value(basedomain);
                item->property_favicon().set_value(image_for_picture_button->get_paintable());
           });
        }
        image_for_picture_button->set(item->property_favicon().get_value());
        dialog_name_entry->set_text(item->property_urlname());
        dialog_url_entry->set_text(item->property_fulldomain().get_value());
        confirm_button->set_label("Update");
    }

    auto spacer = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL);
    spacer->set_expand();
    response_button_box->append(*spacer);
    response_button_box->append(*confirm_button);
    response_button_box->append(*exit_button);

    mainbox->append(*favicon_box);
    mainbox->append(*dialog_name_entry);
    mainbox->append(*dialog_url_entry);
    mainbox->append(*response_button_box);
    dialog->set_child(*mainbox);

    dialog->signal_close_request().connect([]()->bool{
        return false;
    }, false);
    dialog->present();
}

void UrlDropDown::on_entry_text_changed(Gtk::Entry *entry)
{
    if(debounce_conn.connected()) {
        debounce_conn.disconnect();
    }

    debounce_conn = Glib::signal_timeout().connect(
        sigc::bind(sigc::ptr_fun(UrlDropDown::on_debounce_timeout), entry), 1000);
}

bool UrlDropDown::on_debounce_timeout(Gtk::Entry *entry)
{
    entry->set_icon_from_icon_name("process-stop-symbolic", Gtk::Entry::IconPosition::SECONDARY);
    // Cancel previous async request if any
    if (m_url_cancellable) {
        m_url_cancellable->cancel();
        m_url_cancellable.reset();
    }

    m_url_cancellable = Gio::Cancellable::create();
    Glib::ustring entry_text = entry->get_text();
    if (validate_url(entry_text)) {
        // Prepend https if missing
        if (entry_text.rfind("https://", 0) != 0)
           entry_text.insert(0, "https://");

        auto msg = soup_message_new("GET", entry_text.c_str());
        entry->set_data("url", msg);

        soup_session_send_async(m_soup_session,
                                msg,
                                G_PRIORITY_DEFAULT,
                                m_url_cancellable->gobj(),
                                UrlDropDown::send_msg_finished,
                                entry);
    }

    return false;
}

bool UrlDropDown::validate_url(const Glib::ustring &url)
{
    GError *err = nullptr;
    auto guriflags = static_cast<GUriFlags>(G_URI_FLAGS_PARSE_RELAXED | G_URI_FLAGS_NON_DNS);

    auto url_copy = url;

    if (url_copy.empty())
        return false;

    // Reject bare protocol
    if (url_copy == "https://")
        return false;

    // Prepend https if missing
    if (url_copy.rfind("https://", 0) != 0)
        url_copy.insert(0, "https://");

    // Validate full URL
    if (!g_uri_is_valid(url_copy.c_str(), guriflags, &err))
        return false;

    // Parse URI to ensure a hostname exists
    GUri* guri = g_uri_parse(url_copy.c_str(), guriflags, &err);
    if (!guri)
        return false;

    const char* host = g_uri_get_host(guri);
    if (!host || std::strlen(host) == 0) {
        g_uri_unref(guri);
        return false;
    }

    auto psl = psl_builtin();
    if (auto reg_domain = psl_registrable_domain(psl, host); !reg_domain)
        return false;

    if (err)
        g_error_free(err);
    g_uri_unref(guri);

    return true;
}

void UrlDropDown::send_msg_finished(GObject* source_object,
                                    GAsyncResult* res,
                                    const gpointer user_data)
{
    auto session = SOUP_SESSION(source_object);
    auto entry = static_cast<Gtk::Entry*>(user_data);
    auto msg = static_cast<SoupMessage*>(entry->get_data("url"));

    GError *err = nullptr;
    const auto stream = Glib::wrap(soup_session_send_finish(session, res, &err));
    gboolean tried_get = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(msg), "tried_get"));
    auto status = soup_message_get_status(msg);
    if (!stream || SOUP_STATUS_IS_CLIENT_ERROR(status) || SOUP_STATUS_IS_SERVER_ERROR(status)) {
        if (!tried_get) {
            g_object_set_data(G_OBJECT(msg), "tried_get", GINT_TO_POINTER(1));
            soup_message_headers_append(soup_message_get_request_headers(msg), "Range", "bytes=0-0");
            soup_message_set_method(msg, "GET");
            soup_session_send_async(session,
                                    msg,
                                    G_PRIORITY_DEFAULT,
                                    nullptr,
                                    UrlDropDown::send_msg_finished,
                                    entry);
            auto image_missing_texture = Gdk::Texture::create_from_resource("/webling/res/icons/image-missing.svg");
            entry->set_icon_from_paintable(image_missing_texture);
            return;
        }

        // Second strike. It's not happening.
        goto cleanup;
    }

    if (SOUP_STATUS_IS_SUCCESSFUL(status)) {
        auto hostname = g_uri_get_host(soup_message_get_uri(msg));
        auto texture = get_default_favicon(hostname, stream);
        entry->set_icon_from_icon_name("object-select-symbolic", Gtk::Entry::IconPosition::SECONDARY);
        entry->set_data("default-favicon", new Glib::RefPtr<Gdk::Texture>(texture),
            [](void* data) {
            delete static_cast<Glib::RefPtr<Gdk::Texture>*>(data);
        });
    }

cleanup:
    if (err) {
        std::cerr << __PRETTY_FUNCTION__ << ": " << err->message << std::endl;
        g_error_free(err);
    }

    g_object_unref(msg);
}

void UrlDropDown::on_remove_button_pressed() const
{
    auto model = std::dynamic_pointer_cast<Gio::ListStore<UrlItem>>(m_selection->get_model());
    auto item = std::dynamic_pointer_cast<UrlItem>(m_selection->get_selected_item());
    if (!item)
        return;

    auto index = item->get_idx();

    SQLite::Statement stmt_remove(*m_url_db,R"SQL(
                      DELETE FROM "main"."urls"
                      WHERE id = ?
                      )SQL");
    stmt_remove.bind(1, index);
    if (auto nremoved = stmt_remove.exec())
        std::cout << "Removed: " << nremoved << " item(s)." << std::endl;

    model->remove(m_selection->get_selected());
}

void UrlDropDown::setup_listitem_cb(const Glib::RefPtr<Gtk::ListItem>& list_item)
{
    auto box = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL);
    auto favicon_image = Gtk::make_managed<Gtk::Image>();
    favicon_image->set_from_icon_name("image-missing-symbolic");
    auto label = Gtk::make_managed<Gtk::Label>();
    label->add_css_class("listview-item-label");
    auto activated_check = Gtk::make_managed<Gtk::Image>();
    activated_check->set_from_icon_name("object-select-symbolic");
    activated_check->set_opacity(false);

    box->append(*favicon_image);
    box->append(*label);
    box->append(*activated_check);
    list_item->set_child(*box);
}

void UrlDropDown::bind_listitem_cb(const Glib::RefPtr<Gtk::ListItem>& list_item) {
    auto box = list_item->get_child();
    auto box_children = box->get_children();
    auto favicon_image = dynamic_cast<Gtk::Image*>(box_children.at(0));
    auto label = dynamic_cast<Gtk::Label*>(box_children.at(1));
    auto active_mark = dynamic_cast<Gtk::Image*>(box_children.at(2));
    if (!favicon_image || !label || !active_mark) {
        std::cerr << "invalid icon or label or image" << std::endl;
        return;
    }

    auto item = std::dynamic_pointer_cast<UrlItem>(list_item->get_item());
    if (!item)
        return;

    favicon_image->set(item->property_favicon().get_value());
    label->set_text(item->property_urlname());
    if (item->property_fulldomain().get_value() == "")
        label->set_tooltip_text("No url added");
    else label->set_tooltip_text(item->property_fulldomain());

    Glib::Binding::bind_property(item->property_favicon(),
                                 favicon_image->property_paintable());
    Glib::Binding::bind_property(item->property_urlname(),
                                 label->property_label());
    Glib::Binding::bind_property(item->property_fulldomain(),
                                 label->property_tooltip_text());
    Glib::Binding::bind_property(item->property_current_active(),
                                 active_mark->property_opacity(),
                                 Glib::Binding::Flags::DEFAULT,
                                 [](bool is_active)->double {
                                     return is_active ? 1.0 : 0.0;
                                 });
}

void UrlDropDown::activate_cb(guint position) const
{
    auto popover = dynamic_cast<Gtk::Popover*>(m_urls_listview->get_ancestor(GTK_TYPE_POPOVER));

    auto item = std::dynamic_pointer_cast<UrlItem>(m_selection->get_object(position));
    if (!item)
        return;

    auto box = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL);
    box->set_spacing(5);
    auto image = Gtk::make_managed<Gtk::Image>();
    image->set(item->property_favicon().get_value());
    Glib::Binding::bind_property(item->property_favicon(),
        image->property_paintable());
    auto label = Gtk::make_managed<Gtk::Label>(item->property_urlname());
    Glib::Binding::bind_property(item->property_urlname(),
        label->property_label());
    box->append(*image);
    box->append(*label);

    m_menu_button->set_data("url",
                   new Glib::ustring(item->property_fulldomain().get_value()),
                   [](const gpointer data) {
                       delete static_cast<Glib::ustring*>(data);
                   });

    m_menu_button ->set_child(*box);

    for (auto i = 0; i < m_selection->get_n_items(); i++) {
        auto item_in_this_loop = std::dynamic_pointer_cast<UrlItem>(m_selection->get_object(i));
        if (auto current_active_item = std::dynamic_pointer_cast<UrlItem>(
            m_selection->get_object(position)); item_in_this_loop == current_active_item)
            item_in_this_loop->property_current_active().set_value(true);
        else item_in_this_loop->property_current_active().set_value(false);
    }

    popover->popdown();
}

void UrlDropDown::populate_listview(const Glib::RefPtr<Gio::ListStore<UrlItem>> &model) const
{
    SQLite::Statement stmt(*m_url_db, R"SQL(
                      SELECT "urls"."id", "urls"."item_pos", "urls"."name", "urls"."subdomain", "urls"."basedomain", "favicons"."favicon" AS "frn_favicon"
                      FROM "main"."urls"
                      LEFT JOIN favicons ON "urls"."favicon_id" = "favicons"."id"
                      ORDER BY "item_pos" ASC
                      )SQL");

    while(stmt.executeStep()) {
        guint id = stmt.getColumn("id").getUInt();
        Glib::ustring name = stmt.getColumn("name").getString();
        Glib::ustring basedomain = stmt.getColumn("basedomain").getString();
        Glib::ustring subdomain = stmt.getColumn("subdomain").getString();
        auto favicon_id = stmt.getColumn("frn_favicon").getUInt();
        guint item_pos = stmt.getColumn("item_pos").getUInt();

        gconstpointer favicon = nullptr;
        int favicon_size = 0;
        if (!stmt.getColumn("frn_favicon").isNull()) {
            favicon = stmt.getColumn("frn_favicon").getBlob();
            favicon_size = stmt.getColumn("frn_favicon").getBytes();
        }

        auto favicon_texture = Gdk::Texture::create_from_bytes(Glib::Bytes::create(favicon, favicon_size));

        model->append(Glib::make_refptr_for_instance<UrlItem>(new UrlItem(id,
                                                                          item_pos,
                                                                          name,
                                                                          subdomain,
                                                                          basedomain,
                                                                          favicon_id,
                                                                          favicon_texture)));
    }
}

void UrlDropDown::split_urlname(const std::string& url,
                                std::string& subdomain,
                                std::string& basedomain)
{
    auto psl = psl_builtin();
    basedomain = std::string(psl_registrable_domain(psl, url.c_str()));

    subdomain = url;
    if (const auto pos = subdomain.find(basedomain); pos != std::string::npos) {
        if (pos > 0 && subdomain[pos - 1] == '.')
            subdomain.erase(pos - 1); // remove dot and everything after
        else
            subdomain.erase(pos);     // no dot to remove
    }
}

Glib::RefPtr<Gdk::Texture> UrlDropDown::get_default_favicon(const gchar *hostname, const Glib::RefPtr<Gio::InputStream>& stream)
{
    std::string str;
    std::vector<char> buf(4096);

    gssize n = 0;
    while ((n = stream->read(buf.data(), buf.size(), m_url_cancellable)) > 0) {
        str.append(buf.data(), n);
    }

    auto favicon_link = parse_favicon_link_url(str);

    if (favicon_link.rfind("//", 0) == 0)
        favicon_link.insert(0, "https:");
    else if (favicon_link.rfind("/", 0) == 0)
        favicon_link.insert(0, std::string("https://") + hostname);
    else if (favicon_link.rfind("http", 0) != 0)
        favicon_link.insert(0, std::string("https://") + hostname + "/");

    GError *err = nullptr;
    SoupMessage *msg = nullptr;
    GUri *uri = g_uri_parse(favicon_link.c_str(), G_URI_FLAGS_NONE, &err);
    if (uri == nullptr) {
        g_printerr("Skipping invalid favicon URL [%s]: %s\n",
                   favicon_link.c_str(), err->message);
        g_clear_error(&err);
    } else {
        msg = soup_message_new_from_uri("GET", uri);
        g_uri_unref(uri);
    }

    auto favicon_bytes = Glib::wrap(soup_session_send_and_read(m_soup_session, msg, m_url_cancellable->gobj(), &err));
    if (err) {
        std::cerr << __PRETTY_FUNCTION__ << err->message << std::endl;
        g_error_free(err);
        auto texture = Gdk::Texture::create_from_resource("/webling/res/icons/image-missing.svg");
        return Glib::RefPtr<Gdk::Texture>(texture);
    }

    g_object_unref(msg);

    return Gdk::Texture::create_from_bytes(favicon_bytes);
}

std::string UrlDropDown::parse_favicon_link_url(const std::string &str)
{
    auto output = gumbo_parse(str.c_str());
    auto node = output->root;

    const std::vector<std::string> exts {".png", ".svg", ".jpg", ".jpeg", ".ico"};

    for (auto ext : exts) {
        for (auto favicon_link : search_for_favicon_links(node)) {
            if (favicon_link.rfind(ext) != std::string::npos) {
                gumbo_destroy_output(&kGumboDefaultOptions, output);
                return favicon_link;
            }
        }
    }
}

std::vector<std::string> UrlDropDown::search_for_favicon_links(GumboNode* node)
{
    std::vector<std::string> results;

    std::function<void(GumboNode*)> walk = [&](GumboNode* n) {
        if (!n || n->type != GUMBO_NODE_ELEMENT)
            return;

        GumboElement* el = &n->v.element;

        if (el->tag == GUMBO_TAG_LINK) {
            const char* rel = nullptr;
            const char* href = nullptr;

            for (unsigned i = 0; i < el->attributes.length; i++) {
                auto attr = static_cast<GumboAttribute*>(el->attributes.data[i]);
                if (!strcmp(attr->name, "rel"))   rel  = attr->value;
                if (!strcmp(attr->name, "href"))  href = attr->value;
            }

            if (rel && href && strstr(rel, "icon")) {
                results.emplace_back(href);
            }
        }

        GumboVector* children = &el->children;
        for (unsigned i = 0; i < children->length; i++)
            walk(static_cast<GumboNode*>(children->data[i]));
    };

    walk(node);
    return results;
}

Glib::RefPtr<Gdk::Texture> UrlDropDown::decode_image_to_texture(const Glib::RefPtr<Gio::InputStream>& stream)
{
    Glib::RefPtr<Gdk::Texture> texture;
    const auto glyloader = gly_loader_new_for_stream(stream->gobj());
    GError *err = nullptr;
    if (auto glyimage = gly_loader_load(glyloader, &err))
        if (auto glyframe = gly_image_next_frame(glyimage, nullptr))
            texture = Glib::wrap(gly_gtk_frame_get_texture(glyframe));

    if (err) {
        g_printerr("glycin error: %s\n", err->message);
        g_error_free(err);
    }

    return texture;
}

Glib::RefPtr<Gdk::Texture> UrlDropDown::decode_image_to_texture(const Glib::RefPtr<Glib::Bytes>& bytes)
{
    Glib::RefPtr<Gdk::Texture> texture;
    const auto glyloader = gly_loader_new_for_bytes(bytes->gobj());
    GError *err = nullptr;
    if (auto glyimage = gly_loader_load(glyloader, &err)) {
        if (auto glyframe = gly_image_next_frame(glyimage, nullptr)) {
            texture = Glib::wrap(gly_gtk_frame_get_texture(glyframe));
        }
    }
    g_printerr("glycin error: %s\n", err->message);
    return texture;
}