#include "Webling.h"

int main(const int argc, char* argv[]) {
    auto app = Webling::create("com.github.noobaldrin.webling",
                                   Gio::Application::Flags::NONE);
    return app->run(argc, argv);
}
