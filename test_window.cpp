#include <SDL3/SDL.h>
#include <iostream>

int main(int argc, char* argv[]) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "SDL_Init failed" << std::endl;
        return 1;
    }
    
    SDL_Window* window = SDL_CreateWindow("Test Window", 1280, 720, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY);
    if (!window) {
        std::cerr << "SDL_CreateWindow failed: " << SDL_GetError() << std::endl;
        return 1;
    }
    
    int w, h;
    SDL_GetWindowSize(window, &w, &h);
    std::cout << "Created window size: " << w << "x" << h << std::endl;
    
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
