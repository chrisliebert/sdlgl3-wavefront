#include "ConfigLoader.h"
#include <iostream>

int main() {
    ConfigLoader cfg("app.cfg");
    std::cout << "width: " << cfg.getInt("window.width") << std::endl;
    return 0;
}
