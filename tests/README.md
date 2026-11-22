# Unit Tests

This directory contains comprehensive unit and regression tests for the `sdlgl3-wavefront` project.

## Test Suites

The test executable (`unit_tests`) runs 8 distinct suites validating math, spatial indexing, caching, configuration, rendering abstractions, and directional shadow stability:

| Suite | File | Description |
|-------|------|-------------|
| Frustum Culling | `test_frustum.cpp` | Validates frustum plane extraction from view-projection matrices and intersection testing (points, spheres, axis-aligned bounding boxes, and arbitrary convex polygons). |
| Math & Bounding Sphere | `test_math.cpp` | Validates Welzl's exact minimal bounding sphere algorithm ($O(N)$ expected time) on empty inputs, single vertices, 2-point diameters, 3-point planar circles, 4-point tetrahedra, and random point clouds; verifies Ritter fallback; tests uniform and non-uniform matrix transforms and numerical robustness. |
| Cache Round-Trip & Schema | `test_cache.cpp` | Validates chunked binary v2 cache architecture (`.binobj`), `CacheFileHeader` magic (`0x57415643`) and version verification, `ChunkDescriptor` serialization, texture atlas packing metadata, and full round-trip mesh de/serialization. |
| ConfigLoader | `test_config_loader.cpp` | Validates key-value configuration file parsing across all primitives (`bool`, `int`, `float`, `string`), whitespace tolerance, case insensitivity, and comment handling with temporary file fixtures. |
| Camera | `test_camera.cpp` | Validates FPS camera view matrix computation, forward/right/up vector orthogonality, aim rotation, movement integration, pitch clamping to $[-1.55, 1.55]$ radians ($[-89^\circ, 89^\circ]$) preventing gimbal flip, and initial direction/angle synchronization. |
| SceneNode | `test_scenenode.cpp` | Validates scene node graph construction, hierarchical transform propagation, material bindings, move semantics, and vertex/index buffer encapsulation. |
| GpuProgram | `test_gpu_program.cpp` | Validates uniform management (`Mat4`, `Vec3`, `Int`), uniform cache lookups, interface polymorphism, and `std::unique_ptr` lifecycle management in a headless/mocked environment without active GL context. |
| Shadow Projection Stability | `test_shadow.cpp` | Validates directional sunlight matrix stability: Welzl scene bounding sphere anchoring, rotation invariance under camera movement, volume bounds validation ($4096\times 4096$ resolution, $25.0$ distance), Vulkan/OpenGL clip matrix consistency, and shadow bias safety bounds ($[0.00035, 0.002]$) guaranteeing acne elimination and Peter-Panning prevention. |

## Running Tests

### Via CMake and CTest (Recommended)

To build and run all test suites through CTest:

```bash
# Build the test executable
cmake --build build --config Debug --target unit_tests

# Run all test suites via CTest with detailed failure diagnostics
ctest --test-dir build -C Debug --output-on-failure
```

### Direct Execution

You can also execute the standalone test binary directly:

```bash
# Windows
./build/tests/Debug/unit_tests.exe

# Linux / macOS
./build/tests/unit_tests
```

### Via Custom Target

```bash
cmake --build build --config Debug --target run_tests
```

## Test Architecture

- **Zero External Test Dependencies**: Built using standard C++ `<cassert>` and `<iostream>` for instant compilation and minimal build overhead.
- **Context-Agnostic Validation**: Tests are engineered to validate core algorithmic logic, memory safety, and matrix transformations without requiring an active OpenGL or Vulkan context, making them ideal for headless CI/CD environments.
- **Isolated State**: `ConfigLoader` tests generate ephemeral config files with automated teardown to guarantee test independence.

## Adding New Tests

1. Create a new test translation unit (e.g., `tests/test_new_feature.cpp`).
2. Implement a suite runner function conforming to `int runNewFeatureTests()`.
3. Add the forward declaration and entry in the `suites` array in `tests/test_main.cpp`.
4. Append the source file to `TEST_SOURCES` in `tests/CMakeLists.txt`.
5. Build and verify with `ctest --test-dir build -C Debug --output-on-failure`.

## Sample Test Output

```
========================================
  SDLGL3 Wavefront Unit Tests
========================================

--- Running: Frustum Culling ---
=== Frustum Tests ===
  [TEST] Plane extraction from view-projection matrix... PASSED
  [TEST] Point in frustum culling... PASSED
...
[PASS] Frustum Culling completed successfully

--- Running: Shadow Projection Stability ---
=== Directional Shadow Projection Stability Tests ===
  [TEST] Vulkan clip space matrix NDC mapping... PASSED
  [TEST] Rotation invariance: sunlight direction remains fixed during camera rotation... PASSED
  [TEST] Orthographic bounds fit scene volume tightly... PASSED
  [TEST] Shadow bias prevents shadow acne and Peter-Panning... PASSED
[PASS] Shadow Projection Stability completed successfully

========================================
  Results: 8 passed, 0 failed
========================================
```
