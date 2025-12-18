# Webling Browser
    A small easy-access utility type web browser for a single monitor setup. This is the companion app of GNOME Shell extension Webling.

### Use cases (with the GNOME Shell extension please check https://):
- If you don't have multi-monitor or prefer a single monitor setup, this is an easy to show/hide utility app using the GNOME Shell extension indicator button. You can keep your single monitor setup if that's your preference. This still works nicely in multi-monitor setup (the app opens in primary monitor only, can still be moved to other monitors if needed.)
- If you're using an IDE on fullscreen and don't want to lose sight of your code when Alt-tabbing. Or perhaps you have your main web-browser on fullscreen in other workspace, this will save you headache or dizziness from switching workspaces.
- Good for throw-away tabs or for a quick trivial research using AI Chatbots (ChatGPT, Gemini, etc.), this is to avoid cluttering your main web-browser with tabs you won't use later.
- Switching in AI Chatbots from chats to chats takes time. Use tabs for each chats instead (for security reasons maybe you don't want your main web-browser to offer persistent storage for ChatGPT.)

## Dependencies

Webling Browser is a native GNOME application written in C/C++. It depends on the following system libraries and tools:
Runtime / Build Libraries

    gtkmm-4.0
    C++ bindings for GTK 4. Provides the main UI framework.

    webkitgtk-6.0
    WebKitGTK for rendering web content.

    libsoup-3.0
    HTTP client/server library used by WebKitGTK.

    gumbo
    HTML5 parsing library.

    libpsl
    Public Suffix List library, used for correct domain handling.

    glycin-gtk4-2
    Modern image loading framework for GTK4.

    SQLiteCpp
    C++ wrapper around SQLite, used for local storage.

## Installing Dependencies
### Fedora
```
sudo dnf install cmake pkg-config gtkmm4.0-devel webkitgtk6.0-devel libsoup3-devel gumbo-parser-devel libpsl-devel glycin-gtk4-devel sqlitecpp-devel
```

### Debian / Ubuntu

```
sudo apt install cmake pkg-config libgtkmm-4.0-dev libwebkitgtk-6.0-dev libsoup-3.0-dev libgumbo-dev libpsl-dev libglycin-gtk4-2-dev libsqlitecpp-dev
```

## Build & Install
```
git clone https://github.com/noobaldrin/webling
cd webling
cmake -B build -S .
cmake --build build
cmake --install build
```
### This is where the executable and resources are installed, the accompanying GNOME extension expects them there.
```
~/.local/bin
~/.local/share/webling
```
# For bugs, improvements and feature requests please open issues for this repository. Pull requests are welcome, please describe clearly what your changes do.