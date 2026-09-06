#include <SDL3/SDL.h>
#include <iostream>

int main() {
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window* window = SDL_CreateWindow("Test", 1280, 720, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY);
    if (!window) {
        std::cerr << "Window creation failed: " << SDL_GetError() << std::endl;
        return 1;
    }
    int w, h;
    SDL_GetWindowSize(window, &w, &h);
    std::cout << "Window size: " << w << "x" << h << std::endl;
    SDL_GetWindowSizeInPixels(window, &w, &h);
    std::cout << "Window size in pixels: " << w << "x" << h << std::endl;
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
