# Unit Tests

This directory contains unit tests for the SDLGL3 Wavefront project.

## Test Suites

| Suite | File | Description |
|-------|------|-------------|
| Frustum Culling | `test_frustum.cpp` | Tests frustum extraction and culling algorithms (point, sphere, cube, polygon) |
| ConfigLoader | `test_config_loader.cpp` | Tests configuration file parsing (bool, int, float, string, whitespace handling) |
| Camera | `test_camera.cpp` | Tests camera math (aim rotation, movement, perspective/orthographic matrices, vector orthogonality) |
| SceneNode | `test_scenenode.cpp` | Tests scene node construction, naming, material setting, move semantics, vertex data |
| GpuProgram | `test_gpu_program.cpp` | Tests uniform types (Mat4, Vec3, Int), polymorphism, and unique_ptr management |

## Running Tests

### Via CMake (Recommended)

```bash
# Build the tests
cmake --build build --config Debug --target unit_tests

# Run via CTest
cd build
ctest --config Debug

# Run directly
./tests/unit_tests
```

### Via Custom Target

```bash
cmake --build build --config Debug --target run_tests
```

### Via IDE

Open the project in your IDE and run the `unit_tests` target.

## Test Architecture

- Tests use standard C++ `<cassert>` for assertions (no external test framework dependency)
- Each test suite is self-contained with its own runner function
- Tests avoid OpenGL context dependencies where possible (logic-only testing)
- ConfigLoader tests create temporary files that are cleaned up after each test

## Adding New Tests

1. Create a new test file (e.g., `test_new_module.cpp`)
2. Implement a runner function: `int runNewModuleTests()`
3. Add the forward declaration to `test_main.cpp`
4. Add the suite to the `suites` array in `test_main.cpp`
5. Add the source file to `TEST_SOURCES` in `tests/CMakeLists.txt`

## Test Output Format

```
========================================
  SDLGL3 Wavefront Unit Tests
========================================

--- Running: Frustum Culling ---

=== Frustum Tests ===
  [TEST] Plane extraction from view-projection matrix... PASSED
  [TEST] Point in frustum culling... PASSED
...
All Frustum tests PASSED!
[PASS] Frustum Culling completed successfully
```
