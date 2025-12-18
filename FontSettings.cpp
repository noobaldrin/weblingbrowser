#include <iostream>
#include <string>
#include <unordered_set>
#include <set>

#include <gtkmm/window.h>
#include <gtkmm/grid.h>
#include <gtkmm/label.h>
#include <gtkmm/fontdialogbutton.h>
#include <gtkmm/fontdialog.h>
#include <gtkmm/button.h>
#include <gtkmm/box.h>
#include <gtkmm/checkbutton.h>

#include <giomm/settings.h>
#include <pangomm/fontdescription.h>
#include <pangomm/fontfamily.h>

#include <webkit/webkit.h>

#include "FontSettings.h"
#include "Settings.h"

std::vector<WebKitWebView*> *FontSettings::m_webviews = nullptr;
Gtk::FontDialogButton *FontSettings::m_font_btn_default = nullptr;
Gtk::FontDialogButton *FontSettings::m_font_btn_monospace = nullptr;
Gtk::FontDialogButton *FontSettings::m_font_btn_serif = nullptr;
Gtk::FontDialogButton *FontSettings::m_font_btn_sans = nullptr;

FontSettings::FontSettings(std::vector<WebKitWebView*> *webviews)
    : m_mbsettings(Settings::getFontsSettingsInstance())
{
    m_webviews = webviews;
    auto maingrid = Gtk::make_managed<Gtk::Grid>();
    maingrid->set_row_spacing(12);
    maingrid->set_column_spacing(12);
    maingrid->set_margin(12);

    auto font_lbl_default = Gtk::make_managed<Gtk::Label>("Default Font:");
    font_lbl_default->set_halign(Gtk::Align::START);
    font_lbl_default->set_valign(Gtk::Align::CENTER);
    m_font_btn_default = Gtk::make_managed<Gtk::FontDialogButton>();
    m_font_btn_default->set_dialog(Gtk::FontDialog::create());
    m_font_btn_default->set_halign(Gtk::Align::END);
    m_font_btn_default->set_valign(Gtk::Align::CENTER);
    m_font_btn_default->property_font_desc().signal_changed().connect(
        [this] {
        if (!m_overridefonts->get_active())
            return;

        auto fontdesc = m_font_btn_default->get_font_desc();
        js_font_override(FontSettings::FontType::Default);
    });

    auto font_lbl_monospace = Gtk::make_managed<Gtk::Label>("Monospace:");
    font_lbl_monospace->set_halign(Gtk::Align::START);
    font_lbl_monospace->set_valign(Gtk::Align::CENTER);
    m_font_btn_monospace = Gtk::make_managed<Gtk::FontDialogButton>();
    m_font_btn_monospace->set_dialog(Gtk::FontDialog::create());
    m_font_btn_monospace->set_halign(Gtk::Align::END);
    m_font_btn_monospace->set_valign(Gtk::Align::CENTER);
    m_font_btn_monospace->property_font_desc().signal_changed().connect(
        [this] {
         if (!m_overridefonts->get_active())
            return;

        auto fontdesc = m_font_btn_monospace->get_font_desc();
        js_font_override(FontSettings::FontType::Monospace);
    });

    auto font_lbl_serif = Gtk::make_managed<Gtk::Label>("Serif:");
    font_lbl_serif->set_halign(Gtk::Align::START);
    font_lbl_serif->set_valign(Gtk::Align::CENTER);
    m_font_btn_serif = Gtk::make_managed<Gtk::FontDialogButton>();
    m_font_btn_serif->set_dialog(Gtk::FontDialog::create());
    m_font_btn_serif->set_halign(Gtk::Align::END);
    m_font_btn_serif->set_valign(Gtk::Align::CENTER);
    m_font_btn_serif->property_font_desc().signal_changed().connect(
        [this] {
        if (!m_overridefonts->get_active())
            return;

        auto fontdesc = m_font_btn_serif->get_font_desc();
        js_font_override(FontSettings::FontType::Serif);
    });

    auto font_lbl_sans = Gtk::make_managed<Gtk::Label>("Sans Serif:");
    font_lbl_sans->set_halign(Gtk::Align::START);
    font_lbl_sans->set_valign(Gtk::Align::CENTER);
    m_font_btn_sans= Gtk::make_managed<Gtk::FontDialogButton>();
    m_font_btn_sans->set_dialog(Gtk::FontDialog::create());
    m_font_btn_sans->set_halign(Gtk::Align::END);
    m_font_btn_sans->set_valign(Gtk::Align::CENTER);
    m_font_btn_sans->property_font_desc().signal_changed().connect(
        [this] {
        if (!m_overridefonts->get_active())
            return;

        auto fontdesc = m_font_btn_sans->get_font_desc();
        js_font_override(FontSettings::FontType::SansSerif);
    });

    auto save_font_btn = Gtk::make_managed<Gtk::Button>("Save");
    save_font_btn->signal_clicked().connect(
        [this] {
        save_font_settings();
    });

    maingrid->attach(*font_lbl_default, 0, 0, 1, 1);
    maingrid->attach(*m_font_btn_default, 1, 0, 1, 1);

    maingrid->attach(*font_lbl_monospace, 0, 1, 1, 1);
    maingrid->attach(*m_font_btn_monospace, 1, 1, 1, 1);

    maingrid->attach(*font_lbl_serif, 0, 2, 1, 1);
    maingrid->attach(*m_font_btn_serif, 1, 2, 1, 1);

    maingrid->attach(*font_lbl_sans, 0, 3, 1, 1);
    maingrid->attach(*m_font_btn_sans, 1, 3, 1, 1);

    m_overridefonts = Gtk::make_managed<Gtk::CheckButton>();
    m_overridefonts->set_active(true);
    m_overridefonts->set_label("Override font provided by websites");
    maingrid->attach(*m_overridefonts, 1, 4, 1, 1);
    auto response_box = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL);
    auto exit_btn = Gtk::make_managed<Gtk::Button>("Exit");
    exit_btn->signal_clicked().connect(
        [this] {
        hide();
    });
    response_box->append(*exit_btn);

    maingrid->attach(*response_box, 1, 5, 1, 1);
    maingrid->set_halign(Gtk::Align::END);
    maingrid->attach(*save_font_btn, 0, 5, 1, 1);

    set_child(*maingrid);
    set_resizable(false);

    load_font_settings();
}

FontSettings::~FontSettings() noexcept
= default;

void FontSettings::js_font_override(const FontType fonttype) {
    std::string style_id;
    switch (fonttype) {
        case FontType::Default:    style_id = "myfontstyle-default"; break;
        case FontType::Monospace:  style_id = "myfontstyle-mono"; break;
        case FontType::Serif:      style_id = "myfontstyle-serif"; break;
        case FontType::SansSerif:  style_id = "myfontstyle-sans"; break;
    }

    std::string css;

    if (fonttype == FontType::Default) {
        css =
    "   body, *:not(code):not(pre):not(tt):not(kbd) {\n"
    "   font-family: \"" + m_font_btn_default->get_font_desc().get_family() + "\", sans-serif !important;\n"
    "   font-size: 14px !important;\n"
    "   }\n";
    }

    if (fonttype == FontType::Monospace) {
        css =
    "   code, pre, tt, kbd {\n"
    "   font-family: \"" + m_font_btn_monospace->get_font_desc().get_family() + "\", monospace !important;\n"
    "   font-size: 13px !important;\n"
    "}\n";
    }

    if (fonttype == FontType::Serif) {
        css =
    "   h1, h2, h3, .serif {\n"
    "   font-family: \"" + m_font_btn_serif->get_font_desc().get_family() + "\", serif !important;\n"
    "}\n";
    }

    if (fonttype == FontType::SansSerif) {
        css =
    "   .sans, .ui-sans, input, button {\n"
    "   font-family: \"" + m_font_btn_sans->get_font_desc().get_family() + "\", sans-serif !important;\n"
    "}\n";
    }

    std::string js_font_override =
    "   (function() {\n"
    "       let style = document.getElementById(\"" + style_id + "\");\n"
    "       if (!style) {\n"
    "           style = document.createElement(\"style\");\n"
    "           style.id = \"" + style_id + "\";\n"
    "           document.head.appendChild(style);\n"
    "       }\n"
    "       style.innerHTML = `" + css + "`;\n"
    "   })()";

    for (auto wv : *m_webviews)
        webkit_web_view_evaluate_javascript(wv,
                                            js_font_override.c_str(),
                                            -1,
                                            "about:background-inject",
                                            nullptr,
                                            nullptr,
                                            js_font_override_finished,
                                            nullptr);
}

void FontSettings::js_font_override_finished(GObject *source, GAsyncResult *res, gpointer user_data) {}

void FontSettings::save_font_settings() const {
    auto family = m_font_btn_default->get_font_desc().get_family();
    auto size = m_font_btn_default->get_font_desc().get_size() / 1024;
    auto style = get_style_str(m_font_btn_default->get_font_desc().get_style());
    m_mbsettings->set_string("default-font", family + " " + style + " " + std::to_string(size));

    family = m_font_btn_monospace->get_font_desc().get_family();
    size = m_font_btn_monospace->get_font_desc().get_size() / 1024;
    style = get_style_str(m_font_btn_monospace->get_font_desc().get_style());
    m_mbsettings->set_string("monospace", family + " " + style + " " + std::to_string(size));

    family = m_font_btn_serif->get_font_desc().get_family();
    size = m_font_btn_serif->get_font_desc().get_size() / 1024;
    style = get_style_str(m_font_btn_serif->get_font_desc().get_style());
    m_mbsettings->set_string("serif", family + " " + style + " " + std::to_string(size));

    family = m_font_btn_sans->get_font_desc().get_family();
    size = m_font_btn_sans->get_font_desc().get_size() / 1024;
    style = get_style_str(m_font_btn_sans->get_font_desc().get_style());
    m_mbsettings->set_string("sans-serif", family + " " + style + " " + std::to_string(size));
}

void FontSettings::load_font_settings()
{
    auto settings_str = getPangoCompatibleFontName(m_mbsettings->get_string("default-font"));
    auto fontdesc = Pango::FontDescription(settings_str);

    m_font_btn_default->set_font_desc(fontdesc);
    js_font_override(FontType::Default);

    settings_str = getPangoCompatibleFontName(m_mbsettings->get_string("monospace"));
    m_font_btn_monospace->set_font_desc(Pango::FontDescription(settings_str));
    js_font_override(FontType::Monospace);

    settings_str = getPangoCompatibleFontName(m_mbsettings->get_string("serif"));
    m_font_btn_serif->set_font_desc(Pango::FontDescription(settings_str));
    js_font_override(FontType::Serif);

    settings_str = getPangoCompatibleFontName(m_mbsettings->get_string("sans-serif"));
    m_font_btn_sans->set_font_desc(Pango::FontDescription(settings_str));
    js_font_override(FontType::SansSerif);

    m_override_fonts = m_mbsettings->get_boolean("override-button");
}

void FontSettings::on_font_chosen(const Glib::RefPtr<Gio::AsyncResult>& result) const {
    if (!m_overridefonts->get_active())
        return;

    auto fontdesc = m_font_btn_default->get_font_desc();
    js_font_override(FontSettings::FontType::Default);
}


Glib::ustring FontSettings::get_style_str(const Pango::Style style) {
    switch (style) {
        case Pango::Style::ITALIC: return "Italic"; break;
        case Pango::Style::OBLIQUE: return "Oblique"; break;
        case Pango::Style::NORMAL: return "Regular"; break; // skip "Regular"
        default: return ""; break;
    }
}

Glib::ustring FontSettings::get_weight_str(const Pango::Weight weight) {
    switch (weight) {
        case Pango::Weight::THIN:    return "Thin"; break;
        case Pango::Weight::ULTRALIGHT: return "ExtraLight"; break;
        case Pango::Weight::LIGHT:  return "Light"; break;
        case Pango::Weight::NORMAL: return "Regular"; break;
        case Pango::Weight::MEDIUM: return "Medium"; break;
        case Pango::Weight::SEMIBOLD: return "SemiBold"; break;
        case Pango::Weight::BOLD:    return "Bold"; break;
        case Pango::Weight::ULTRABOLD:  return "ExtraBold"; break;
        case Pango::Weight::HEAVY:   return "Heavy"; break;
        default:                            return ""; break;
    }
}

std::string FontSettings::getPangoCompatibleFontName(const std::string& font_name)
{
    static const std::unordered_set<std::string> style_keywords = {
        "Italic", "Oblique", "Roman", "Normal",
        "Thin", "UltraLight", "Light", "Regular", "Medium",
        "SemiBold", "Bold", "UltraBold", "Heavy", "Black",
        "Condensed", "Expanded", "SemiCondensed", "ExtraCondensed",
        "SemiExpanded", "ExtraExpanded"
    };

    std::istringstream iss(font_name);
    std::vector<std::string> tokens;
    std::string word;

    while(iss >> word) {
        tokens.push_back(word);
    }

    if (tokens.empty()) return "";

    std::string font_size;
    std::string style_parts;
    std::vector<std::string> family_parts;

    // Take the numeric part as font size
    if (!tokens.empty() &&
        std::all_of(tokens.back().begin(), tokens.back().end(), ::isdigit)) {
            font_size = tokens.back();
            tokens.pop_back();
    }

    // The next string should be the style
    if (!tokens.empty()) {
        if (style_keywords.find(tokens.back()) != style_keywords.end()) {
            style_parts = tokens.back();
            tokens.pop_back();
        }
    }

    // The remaining part is the family name
    std::set<std::string> available_font_names;
    auto pango_context = this->get_pango_context();
    for (const auto& available_font_family : pango_context->list_families())
        available_font_names.insert(available_font_family->get_name());

    std::ostringstream oss;
    for (auto &token : tokens) {
        if (token != tokens.back())
            oss << token << " ";
        else oss << token;
    }

    // Add a comma to be compatible with FontDescription format
    if (available_font_names.find(oss.str()) != available_font_names.end())
        oss << ", ";

    // Reassemble font names, style and size
    if (!style_parts.empty())
        oss << style_parts << " ";

    if (!font_size.empty())
        oss << font_size;

    return oss.str();

}
