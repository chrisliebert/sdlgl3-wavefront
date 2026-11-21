# sdlgl3-wavefront

sdlgl3-wavefront is a lightweight 3D renderer starter focused on Wavefront OBJ loading, scene culling, and shader-based rendering. The current codebase targets OpenGL 3.3+ and uses SDL3 plus SDL3_image via CMake.

## Project goals

- Keep the core runtime small and understandable.
- Run well on older hardware (OpenGL 3.3 baseline).
- Add optional fast paths for newer GPUs without breaking baseline compatibility.
- Provide a top-down architecture path for production growth.

See the production playbook: [doc/Production_Playbook.md](doc/Production_Playbook.md)

## Build

This project can auto-fetch SDL3 and SDL3_image when not installed locally.

### Recommended (CMake presets)

Configure:

```bash
cmake --preset ninja-debug
```

Build:

```bash
cmake --build --preset build-ninja-debug
```

Run:

```bash
./build/ninja-debug/sdlglapp
```

### Manual configure/build

```bash
cmake -S . -B build -DSDL3_FETCH_IF_MISSING=ON
cmake --build build --config Debug
```

## Architecture direction

The production direction is to preserve a simple top-down flow:

1. Platform and app loop
2. Scene update and visibility
3. Frame graph and pass scheduling
4. Render backend (OpenGL today, backend abstraction for SDL GPU API next)

This keeps design lightweight while enabling modern hardware features where available.

## Screenshots

![screenshot 1](https://raw.githubusercontent.com/chrisliebert/sdlgl3-wavefront/master/sdlgl3-wavefront_screenshot1.jpg)
![screenshot 2](https://raw.githubusercontent.com/chrisliebert/sdlgl3-wavefront/master/sdlgl3-wavefront_screenshot2.jpg)
![screenshot 3](https://raw.githubusercontent.com/chrisliebert/sdlgl3-wavefront/master/sdlgl3-wavefront_screenshot3.jpg)
![screenshot 4](https://raw.githubusercontent.com/chrisliebert/sdlgl3-wavefront/master/sdlgl3-wavefront_screenshot4.jpg)
![screenshot 5](https://raw.githubusercontent.com/chrisliebert/sdlgl3-wavefront/master/sdlgl3-wavefront_screenshot5.jpg)
![screenshot 6](https://raw.githubusercontent.com/chrisliebert/sdlgl3-wavefront/master/sdlgl3-wavefront_screenshot6.jpg)

## License

sdlgl3-wavefront is licensed under the BSD 2-clause license.

Dependencies keep their original licenses:

- GLM: MIT
- SDL3 and SDL3_image: zlib
