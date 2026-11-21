# sdlgl3-wavefront: Remediation Plan and Implementation Guide

**Prepared for:** PhD Review Board  
**Date:** 2026-08-30  
**Reference:** [`phd-defense-assessment.md`](./phd-defense-assessment.md)  

---

## Executive Summary

This remediation plan addresses the **9 critical (P0)** and **21 high-priority (P1)** findings from the PhD defense-style assessment. The plan is organized into four implementation phases, each with specific deliverables, verification criteria, and acceptance tests.

### Current State Summary

| Category | Count | Status |
|----------|-------|--------|
| P0 Critical (Open) | 3 | Requires immediate action |
| P1 High (Open) | 5 | Sprint 1-2 priority |
| P2 Medium (Open) | 6 | Sprint 3-4 priority |
| P3 Low (Open) | 4 | Sprint 5-6 priority |
| Previously Fixed | ~50 | Verified in assessment |

---

## Phase 1: Critical Fixes (Immediate -- P0 Items)

### Fix 1.1: Frustum Left/Right Plane Sign Convention

**File:** [`src/Frustum.cpp`](src/Frustum.cpp):31-42  
**Risk:** Latent defect in asymmetric frustum culling  

**Root Cause:** The frustum plane extraction formula swaps the `+` and `-` operators for left (XNEG) and right (XPOS) planes after matrix transpose.

**Implementation:**
```cpp
// BEFORE (lines 31-42):
// Left plane -- uses + (WRONG)
planes[PLANE_XNEG][0] = m[3][0] + m[0][0];
// Right plane -- uses - (WRONG)
planes[PLANE_XPOS][0] = m[3][0] - m[0][0];

// AFTER:
// Left plane -- uses - (CORRECT for left plane extraction)
planes[PLANE_XNEG][0] = m[3][0] - m[0][0];
// Right plane -- uses + (CORRECT for right plane extraction)
planes[PLANE_XPOS][0] = m[3][0] + m[0][0];
```

**Verification:**
1. Build: `cmake --build build --config Debug`
2. Load a scene with asymmetric camera projection
3. Verify frustum culling correctly identifies visible nodes on both left and right sides

---

### Fix 1.2: Remove OpenGL 4.6 Fallback Candidate

**File:** [`src/OpenGLBackend.cpp`](src/OpenGLBackend.cpp):47  
**Risk:** Context creation failure on drivers that reject invalid version requests  

**Root Cause:** OpenGL 4.6 does not exist as a separate profile. The highest available core profile is 4.5.

**Implementation:**
```cpp
// BEFORE:
constexpr std::array<GlContextVersion, 4> candidates = {{
    {4, 6}, {4, 5}, {4, 3}, {3, 3}
}};

// AFTER:
constexpr std::array<GlContextVersion, 4> candidates = {{
    {4, 5}, {4, 3}, {3, 3}, {4, 6}  // Try 4.5 first, fall back to 4.6 last (may fail gracefully)
}};
```

**Verification:**
1. Build and launch on multiple driver versions
2. Verify context creation succeeds without warnings
3. Confirm reported OpenGL version matches highest supported

---

### Fix 1.3: Add OBJ File Size Limits

**File:** [`src/Renderer.cpp`](src/Renderer.cpp):294-725 (`addWavefront`)  
**Risk:** Memory exhaustion from maliciously large OBJ files  

**Root Cause:** No validation of vertex/face counts before allocation.

**Implementation:**
```cpp
// Add after tinyobj::ObjReader reader; (around line 339):
constexpr size_t MAX_VERTICES = 100'000'000ULL;   // 100M vertices ~ 7.2GB
constexpr size_t MAX_FACES = 50'000'000ULL;        // 50M faces ~ 3.6GB

if (attrib.vertices.size() > MAX_VERTICES) {
    std::cerr << "OBJ rejected: vertex count " << attrib.vertices.size()
              << " exceeds limit " << MAX_VERTICES << std::endl;
    return;
}
if (shapes.empty()) {
    std::cerr << "OBJ rejected: no shapes found" << std::endl;
    return;
}

size_t totalFaces = 0;
for (const auto& s : shapes) {
    for (size_t f = 0; f < s.mesh.num_face_vertices.size(); ++f) {
        totalFaces += s.mesh.num_face_vertices[f];
        if (totalFaces > MAX_FACES) {
            std::cerr << "OBJ rejected: face count " << totalFaces
                      << " exceeds limit " << MAX_FACES << std::endl;
            return;
        }
    }
}
```

**Verification:**
1. Load a normal OBJ file -- should succeed
2. Load a synthetic OBJ with 200M vertices -- should reject with clear error
3. Verify memory usage stays bounded for large inputs

---

## Phase 2: Architecture Improvements (Sprint 1-2 -- P1 Items)

### Fix 2.1: Implement True Scene Graph Hierarchy

**Files:** [`include/SceneNode.h`](include/SceneNode.h), [`include/Renderer.h`](include/Renderer.h)  
**Effort:** 8 hours  
**Risk:** Design debt limiting scalability for complex scenes  

**Design:**
```cpp
// New hierarchy structure (add to include/SceneGraph.h):
class SceneGraphNode {
public:
    std::string name;
    glm::vec3 localTranslation{0, 0, 0};
    glm::quat localRotation{1, 0, 0, 0};
    glm::vec3 localScale{1, 1, 1};
    mutable glm::mat4 worldTransform;
    mutable bool worldTransformDirty = true;
    
    std::weak_ptr<SceneGraphNode> parent;
    std::vector<std::shared_ptr<SceneGraphNode>> children;
    SceneNode sceneData;  // Existing geometry data
    
    void markDirty() const { worldTransformDirty = true; }
    void refreshWorldTransform() const;
    glm::mat4 getWorldTransform() const;
    
    std::shared_ptr<SceneGraphNode> addChild(std::shared_ptr<SceneGraphNode> child);
    bool removeChild(std::shared_ptr<SceneGraphNode> child);
    std::shared_ptr<SceneGraphNode> findNodeByName(const std::string& name);
};
```

**Verification:**
1. Create a scene with parent-child hierarchy
2. Verify world transforms compute correctly through the chain
3. Verify culling works with hierarchical bounding volumes

---

### Fix 2.2: Wire ShaderCache into Rendering Path

**Files:** [`src/OpenGLBackend.cpp`](src/OpenGLBackend.cpp):356-388, [`include/Shader.h`](include/Shader.h)  
**Effort:** 4 hours  

**Implementation:**
```cpp
// In OpenGLBackend::bufferToGpu(), replace direct Shader construction:
// BEFORE:
auto shadowVert = std::make_shared<Shader>(depthVertShaderPath, GL_VERTEX_SHADER);

// AFTER:
auto& shaderCache = ShaderCache::getInstance();
auto shadowVert = shaderCache.getShader(depthVertShaderPath);
if (!shadowVert || !shadowVert->isCompiled()) {
    std::cerr << "Failed to get cached shadow vertex shader" << std::endl;
    return false;
}
```

**Verification:**
1. Load a scene -- shaders compile on first call
2. Reload the same scene -- shaders load from cache (verify via ShaderCache stats)
3. Modify a shader file -- verify cache invalidation triggers recompilation

---

### Fix 2.3: Add Shadow Frustum Culling

**Files:** [`src/OpenGLBackend.cpp`](src/OpenGLBackend.cpp):480-488  
**Effort:** 3 hours  

**Implementation:**
```cpp
// In createShadowMap(), filter nodes before rendering:
std::vector<SceneNode*> culledNodes;
for (const auto* node : nodes) {
    // Check if node bounding sphere intersects light frustum
    if (frustum.spherePartiallyInFrustum(node->lx, node->ly, node->lz, node->boundingSphere)) {
        culledNodes.push_back(node);
    }
}

// Render only culled nodes:
for (const auto* node : culledNodes) { ... }
```

**Verification:**
1. Load a large scene with objects outside the light frustum
2. Verify shadow map renders correctly
3. Measure performance improvement for large scenes

---

### Fix 2.4: Replace void* Thread Handle with SDL_Thread*

**Files:** [`include/Renderer.h`](include/Renderer.h):141, [`src/Renderer.cpp`](src/Renderer.cpp)  
**Effort:** 1 hour  

**Implementation:**
```cpp
// In include/Renderer.h:
// BEFORE:
void* binCacheWriterThread = nullptr;

// AFTER:
#if __has_include(<SDL3/SDL_thread.h>)
#include <SDL3/SDL_thread.h>
#else
#include <SDL_thread.h>
#endif
SDL_Thread* binCacheWriterThread = nullptr;
```

**Verification:**
1. Build with SDL3 headers -- verify type compatibility
2. Trigger cache write -- verify thread creation and cleanup work correctly

---

### Fix 2.5: Add Configurable VSync

**Files:** [`src/OpenGLBackend.cpp`](src/OpenGLBackend.cpp):70, [`config/renderer.cfg`](config/renderer.cfg)  
**Effort:** 1 hour  

**Implementation:**
```cpp
// In OpenGLBackend::initialize():
bool vsyncEnabled = config->hasVar("renderer.vsync") ? config->getBool("renderer.vsync") : false;
SDL_GL_SetSwapInterval(vsyncEnabled ? 1 : 0);
```

**Verification:**
1. Set `renderer.vsync = true` in config -- verify VSync enabled
2. Set `renderer.vsync = false` -- verify VSync disabled
3. Verify no tearing with VSync on, no frame limiting with VSync off

---

## Phase 3: Performance Optimization (Sprint 3-4 -- P2 Items)

### Fix 3.1: Vertex Cache Optimization

**Files:** [`src/Renderer.cpp`](src/Renderer.cpp):601-719  
**Effort:** 4 hours  

**Approach:** Add vertex deduplication during OBJ loading using a hash map from vertex key to index.

### Fix 3.2: Improve Camera Resize Handling

**Files:** [`src/Camera.cpp`](src/Camera.cpp)  
**Effort:** 2 hours  

**Approach:** Add `Camera::updateProjection()` method that recomputes the projection matrix from current viewport dimensions. Call from window resize event handler.

### Fix 3.3: Fix Texture Format Detection

**Files:** [`src/Renderer.cpp`](src/Renderer.cpp):103-108  
**Effort:** 3 hours  

**Approach:** Use SDL surface format flags to determine the correct GL internal format instead of bpp heuristic.

---

## Phase 4: Hardening (Sprint 5-6 -- P3 Items)

### Fix 4.1: Remove Dead Code

**Files:** [`src/SceneNode.cpp`](src/SceneNode.cpp)  
**Effort:** 30 minutes  

**Action:** Delete the file or populate with actual SceneNode methods.

### Fix 4.2: Add Unit Tests

**Files:** `tests/` directory  
**Effort:** 12 hours  

**Test Suite:**
1. Frustum culling correctness (point, sphere, cube tests)
2. Bounding sphere computation (known geometries)
3. Binary cache serialization/deserialization round-trip
4. ConfigLoader parsing edge cases
5. Path traversal protection

### Fix 4.3: Replace _CRT_SECURE_NO_WARNINGS

**Files:** [`CMakeLists.txt`](CMakeLists.txt):20, [`src/main.cpp`](src/main.cpp):1  
**Effort:** 2 hours  

**Action:** Replace remaining `sprintf_s` usage with `std::format` (C++20) or `snprintf`. Remove the macro definition.

### Fix 4.4: Fix Shadow Map Border Color

**Files:** [`src/OpenGLBackend.cpp`](src/OpenGLBackend.cpp):444  
**Effort:** 30 minutes  

**Implementation:**
```cpp
// BEFORE:
constexpr GLfloat borderColor[] = {1.0f, 1.0f, 1.0f, 1.0f};

// AFTER (fully shadowed border for safe fallback):
constexpr GLfloat borderColor[] = {0.0f, 0.0f, 0.0f, 0.0f};
```

---

## Verification Matrix

| Phase | Deliverable | Verification Command | Acceptance Criteria |
|-------|-------------|---------------------|---------------------|
| 1.1 | Frustum sign fix | Build + asymmetric camera test | Culling correct on both sides |
| 1.2 | OpenGL version fix | Build + launch on multiple drivers | Context creation succeeds |
| 1.3 | OBJ size limits | Load large synthetic OBJ | Rejected with clear error |
| 2.1 | Scene graph hierarchy | Create parent-child scene | World transforms correct |
| 2.2 | ShaderCache wiring | Load/reload scene | Cache stats show hits |
| 2.3 | Shadow frustum culling | Large scene outside light view | Performance improvement measured |
| 2.4 | Thread handle type | Build with SDL3 | Type compatibility verified |
| 2.5 | Configurable VSync | Toggle via config | VSync state matches config |
| 3.x | Performance fixes | Benchmark before/after | Measurable improvement |
| 4.x | Hardening | Unit test suite | All tests pass |

---

## Risk Assessment

| Risk | Probability | Impact | Mitigation |
|------|-------------|--------|------------|
| Frustum fix breaks symmetric culling | Low | Medium | Test with symmetric camera first |
| Scene graph hierarchy breaks existing code | Medium | High | Implement as opt-in feature |
| ShaderCache wiring introduces bugs | Low | Medium | Start with shadow shaders only |
| OBJ size limits reject valid files | Low | Low | Set limits conservatively |

---

## Success Criteria

The remediation plan is considered complete when:

1. All P0 items are fixed and verified
2. All P1 items are implemented and tested
3. Build passes on Windows (MSVC), Linux (GCC/Clang)
4. Unit test suite achieves >80% coverage of core algorithms
5. Performance benchmarks show no regression vs. baseline

---

*Prepared for PhD Review Board consideration.*  
*Date: 2026-08-30*  
*Reference: PhD Defense Assessment (phd-defense-assessment.md)*
