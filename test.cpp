#include <iostream>
#include <string>
#include <string_view>
#include <filesystem>

int main() {
    std::string_view sv = "shadow_mapping_depth.vs";
    std::cout << (std::filesystem::path("shaders") / std::string(sv)).string() << std::endl;
    return 0;
}
