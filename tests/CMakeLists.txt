# ============================================================================
# Unit Test Configuration
# ============================================================================

enable_testing()

# Test executable
set(TEST_SOURCES
    test_main.cpp
    test_frustum.cpp
    test_config_loader.cpp
    test_camera.cpp
    test_scenenode.cpp
    test_gpu_program.cpp
    ../src/Frustum.cpp
    ../src/ConfigLoader.cpp
    ../src/GpuProgram.cpp
    ../src/Shader.cpp
)

add_executable(unit_tests ${TEST_SOURCES})

target_include_directories(unit_tests PRIVATE ../include)
target_include_directories(unit_tests PRIVATE ../dependencies/glm)

# Link dependencies
target_link_libraries(unit_tests PRIVATE
    ${GLAD_LINK_TARGET}
    OpenGL::GL
    ${SDL_LINK_LIBS}
)

# Copy config files to build directory for tests
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/../config DESTINATION ${CMAKE_BINARY_DIR}/tests)

# Add test to CTest
add_test(
    NAME unit_tests
    COMMAND unit_tests
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/tests
)

# Custom target to run tests
add_custom_target(run_tests
    COMMAND unit_tests
    DEPENDS unit_tests
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/tests
    COMMENT "Running unit tests..."
)
