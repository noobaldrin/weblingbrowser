#include <iostream>

#include <SQLiteCpp/Exception.h>
#include "Webling.h"

int main(const int argc, char* argv[]) {
    try {
        auto app = Webling::create("com.github.noobaldrin.webling",
                                       Gio::Application::Flags::NONE);
        return app->run(argc, argv);
    } catch (const SQLite::Exception& e) {
        std::cerr << e.what() << std::endl;
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
    }
}
