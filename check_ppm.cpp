#include <iostream>
#include <fstream>
#include <string>

int main() {
    std::ifstream file("mcp_capture.ppm", std::ios::binary);
    if (!file) return 1;
    std::string header;
    file >> header;
    int w, h, max_val;
    file >> w >> h >> max_val;
    file.get(); // skip newline
    
    int pink_count = 0;
    int other_count = 0;
    
    for (int i = 0; i < w * h; ++i) {
        unsigned char r, g, b;
        file.read(reinterpret_cast<char*>(&r), 1);
        file.read(reinterpret_cast<char*>(&g), 1);
        file.read(reinterpret_cast<char*>(&b), 1);
        if (r == 255 && g == 204 && b == 204) {
            pink_count++;
        } else {
            other_count++;
        }
    }
    std::cout << "Pink pixels: " << pink_count << "\nOther pixels: " << other_count << std::endl;
    return 0;
}
