// Main test runner - includes all test suites
#include <iostream>

// Forward declarations of test runners
int runFrustumTests();
int runConfigLoaderTests();
int runCameraTests();
int runSceneNodeTests();
int runGpuProgramTests();

struct TestSuite {
    const char* name;
    int (*testFunc)();
};

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "  SDLGL3 Wavefront Unit Tests" << std::endl;
    std::cout << "========================================" << std::endl;
    
    TestSuite suites[] = {
        {"Frustum Culling", runFrustumTests},
        {"ConfigLoader", runConfigLoaderTests},
        {"Camera", runCameraTests},
        {"SceneNode", runSceneNodeTests},
        {"GpuProgram", runGpuProgramTests},
    };
    
    int totalPassed = 0;
    int totalFailed = 0;
    
    for (const auto& suite : suites) {
        std::cout << "\n--- Running: " << suite.name << " ---" << std::endl;
        int result = suite.testFunc();
        if (result == 0) {
            totalPassed++;
            std::cout << "[PASS] " << suite.name << " completed successfully" << std::endl;
        } else {
            totalFailed++;
            std::cout << "[FAIL] " << suite.name << " had " << result << " failures" << std::endl;
        }
    }
    
    std::cout << "\n========================================" << std::endl;
    std::cout << "  Results: " << totalPassed << " passed, " 
              << totalFailed << " failed" << std::endl;
    std::cout << "========================================" << std::endl;
    
    return totalFailed > 0 ? 1 : 0;
}
