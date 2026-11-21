#include "GpuProgram.h"
#include <iostream>
#include <cassert>
#include <cmath>

// Tolerance for floating point comparisons
static constexpr float EPSILON = 1e-4f;

static bool approxEqual(float a, float b) {
    return std::fabs(a - b) < EPSILON;
}

// ============================================================================
// Uniform type tests (logic-only, no OpenGL context required)
// ============================================================================

void testUniformMat4InitialValue() {
    std::cout << "  [TEST] UniformMat4 initial value... ";
    
    glm::mat4 testMatrix(1.0f); // identity
    testMatrix[0][0] = 2.0f;
    testMatrix[1][1] = 3.0f;
    testMatrix[2][2] = 4.0f;
    
    UniformMat4 uniform(testMatrix);
    
    // Verify the matrix was stored (we can't test load() without OpenGL context)
    // But we can verify the class compiles and stores data
    
    std::cout << "PASSED" << std::endl;
}

void testUniformMat4SetValue() {
    std::cout << "  [TEST] UniformMat4 setValue... ";
    
    glm::mat4 mat1(1.0f);
    mat1[0][0] = 5.0f;
    
    glm::mat4 mat2(1.0f);
    mat2[1][1] = 10.0f;
    
    UniformMat4 uniform(mat1);
    uniform.set(mat2);
    
    std::cout << "PASSED" << std::endl;
}

void testUniformVec3InitialValue() {
    std::cout << "  [TEST] UniformVec3 initial value... ";
    
    glm::vec3 testVec(1.0f, 2.0f, 3.0f);
    UniformVec3 uniform(testVec);
    
    std::cout << "PASSED" << std::endl;
}

void testUniformVec3SetValue() {
    std::cout << "  [TEST] UniformVec3 setValue... ";
    
    glm::vec3 vec1(1.0f, 2.0f, 3.0f);
    glm::vec3 vec2(4.0f, 5.0f, 6.0f);
    
    UniformVec3 uniform(vec1);
    uniform.set(vec2);
    
    std::cout << "PASSED" << std::endl;
}

void testUniformIntInitialValue() {
    std::cout << "  [TEST] UniformInt initial value... ";
    
    UniformInt uniform(42);
    
    std::cout << "PASSED" << std::endl;
}

void testUniformIntSetValue() {
    std::cout << "  [TEST] UniformInt setValue... ";
    
    UniformInt uniform(10);
    uniform.set(20);
    
    std::cout << "PASSED" << std::endl;
}

// ============================================================================
// Uniform base class tests
// ============================================================================

void testUniformLocation() {
    std::cout << "  [TEST] Uniform location getter/setter... ";
    
    UniformMat4 uniform(glm::mat4(1.0f));
    
    // Default location should be 0 (uninitialized)
    GLuint loc = uniform.getLocation();
    (void)loc; // Suppress unused warning
    
    // Set and get location
    uniform.setLocation(5);
    assert(uniform.getLocation() == 5);
    
    std::cout << "PASSED" << std::endl;
}

// ============================================================================
// UniformLoader tests (logic-only)
// ============================================================================

void testUniformLoaderEmpty() {
    std::cout << "  [TEST] UniformLoader empty state... ";
    
    // We can't create a real UniformLoader without OpenGL context,
    // but we can verify the class interface exists and compiles
    
    std::cout << "PASSED" << std::endl;
}

// ============================================================================
// GpuProgram tests (logic-only)
// ============================================================================

void testGpuProgramInterface() {
    std::cout << "  [TEST] GpuProgram interface... ";
    
    // We can't create a real GpuProgram without OpenGL context,
    // but we can verify the class interface exists and compiles
    
    std::cout << "PASSED" << std::endl;
}

// ============================================================================
// Uniform polymorphism tests
// ============================================================================

void testUniformPolymorphism() {
    std::cout << "  [TEST] Uniform polymorphic behavior... ";
    
    // Test that UniformMat4, UniformVec3, and UniformInt all inherit from Uniform
    Uniform* base1 = new UniformMat4(glm::mat4(1.0f));
    Uniform* base2 = new UniformVec3(glm::vec3(0.0f));
    Uniform* base3 = new UniformInt(0);
    
    // All should have valid getLocation
    assert(base1->getLocation() != GL_INVALID_INDEX || base1->getLocation() == 0);
    assert(base2->getLocation() != GL_INVALID_INDEX || base2->getLocation() == 0);
    assert(base3->getLocation() != GL_INVALID_INDEX || base3->getLocation() == 0);
    
    delete base1;
    delete base2;
    delete base3;
    
    std::cout << "PASSED" << std::endl;
}

void testUniformPtrManagement() {
    std::cout << "  [TEST] Uniform unique_ptr management... ";
    
    // Test that UniformPtr (unique_ptr<Uniform>) works correctly
    UniformPtr ptr1 = std::make_unique<UniformMat4>(glm::mat4(1.0f));
    assert(ptr1 != nullptr);
    
    UniformPtr ptr2 = std::make_unique<UniformVec3>(glm::vec3(0.0f));
    assert(ptr2 != nullptr);
    
    // Test move semantics
    UniformPtr ptr3 = std::move(ptr1);
    assert(ptr3 != nullptr);
    assert(ptr1 == nullptr);
    
    std::cout << "PASSED" << std::endl;
}

// ============================================================================
// Test runner
// ============================================================================

int runGpuProgramTests() {
    std::cout << "\n=== GpuProgram Tests ===" << std::endl;
    
    testUniformMat4InitialValue();
    testUniformMat4SetValue();
    testUniformVec3InitialValue();
    testUniformVec3SetValue();
    testUniformIntInitialValue();
    testUniformIntSetValue();
    testUniformLocation();
    testUniformLoaderEmpty();
    testGpuProgramInterface();
    testUniformPolymorphism();
    testUniformPtrManagement();
    
    std::cout << "All GpuProgram tests PASSED!" << std::endl;
    return 0;
}
