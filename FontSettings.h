#ifndef WEBLING_FONT_SETTINGS_H
#define WEBLING_FONT_SETTINGS_H

#include <vector>
#include <memory>
#include <gtkmm/window.h>

namespace Gtk {
    class Widget;
    class CheckButton;
    class FontDialogButton;
}

namespace Gio {
    class Settings;
}

namespace Pango {
    class FontDescription;
}

typedef struct _WebKitSettings WebKitSettings;
typedef struct _WebKitWebView WebKitWebView;

class FontSettings final : public Gtk::Window {
public:
    enum class FontType {
        Default,
        Monospace,
        Serif,
        SansSerif
    };

    FontSettings(std::vector< WebKitWebView* >* webviews);
    ~FontSettings() noexcept override;


    static void js_font_override(FontType fonttype);
    void save_font_settings() const;
    void load_font_settings();

private:
    static Glib::ustring get_style_str(Pango::Style style);
    static Glib::ustring get_weight_str(Pango::Weight weight);
    static void js_font_override_finished(GObject *source, GAsyncResult *res, gpointer user_data);

    void on_font_chosen(const Glib::RefPtr<Gio::AsyncResult>& result) const;
    std::string getPangoCompatibleFontName(const std::string &font_name);

    static std::vector<WebKitWebView*> *m_webviews;

    Gtk::CheckButton *m_overridefonts;
    std::shared_ptr<Gio::Settings> m_mbsettings;

    static Gtk::FontDialogButton *m_font_btn_default;
    static Gtk::FontDialogButton *m_font_btn_monospace;
    static Gtk::FontDialogButton *m_font_btn_serif;
    static Gtk::FontDialogButton *m_font_btn_sans;

    gboolean m_override_fonts;
};

#endif // WEBLING_FONT_SETTINGS_H
