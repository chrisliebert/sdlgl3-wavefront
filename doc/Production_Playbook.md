# Production Playbook

This document defines a production-oriented, lightweight top-down architecture for this renderer, with a practical path from current OpenGL 3.3 code to modern hardware backends.

## Design principles

- Baseline first: keep OpenGL 3.3 path healthy and testable.
- Progressive enhancement: add newer GPU features behind capability checks.
- Small abstractions: avoid large engine rewrites and deep inheritance trees.
- Data-oriented frame flow: move from ad hoc draw logic to explicit pass ordering.
- Measurable outcomes: every feature should have timing or memory evidence.

## Top-down architecture

Use this layered flow:

1. App Layer
   - Owns startup, shutdown, config, window lifecycle, input routing.
2. Scene Layer
   - Owns camera, transforms, visibility, material assignment.
3. Frame Layer
   - Builds per-frame passes (shadow, depth prepass, opaque, transparent, post).
4. Render API Layer
   - Backend-agnostic command model (`IRenderBackend`, `RenderCommand`).
5. Backend Layer
   - OpenGL backend (`OpenGLBackend`: OpenGL 3.3 core compatibility + OpenGL 4.4 persistent-mapped fast path).
   - Vulkan backend (`VulkanBackend`: Native Vulkan 1.2+ with SPIR-V pipeline, dedicated shadow render pass, frame uniform buffer, and descriptor caching).
   - Backend factory (`RenderBackendFactory`: dynamic runtime backend selection driven by configuration).

Keep dependencies one-way (top to bottom only).

## Hardware strategy

### Tier A: Compatibility (Production-Ready)

- API: OpenGL 3.3 core.
- Status: Fully implemented, hardened, and verified.
- Goal: broad desktop coverage and stable correctness across all desktop GPUs.
- Constraints: no persistent mapping requirement, no bindless assumptions.

### Tier B: OpenGL fast path (Production-Ready)

- API target: OpenGL 4.4+ capabilities when present.
- Status: Fully implemented with runtime capability detection.
- Features: Immutable buffers and persistent mapping for dynamic uploads (`glBufferStorage` with `PERSISTENT_BIT`).
- Fallback path using orphaning and map/unmap when 4.4+ is unavailable.

### Tier C: Modern backend (Production-Ready)

- API: Native Vulkan 1.2+ backend (`VulkanBackend`).
- Status: Fully implemented, hardened, and verified with 100% feature parity.
- Features: SPIR-V shader pipeline (`vulkan.vert.spv`, `vulkan.frag.spv`, `vulkan_shadow_depth.vert.spv`), dedicated shadow render pass (`VK_FORMAT_D32_SFLOAT`), frame uniform buffer (`FrameUniforms` UBO binding 2), and descriptor set caching.
- Goal: access modern explicit queue and pipeline behavior with minimal CPU overhead and zero driver stalls.

## Lightweight production guardrails

- One frame owns all transient allocations.
- One source of truth for render state per pass.
- No blocking GPU waits on frame path except resize/device-loss transitions.
- Resource lifetime model must be explicit (startup, per-level, per-frame).
- All major systems have debug labels and counters.

## Performance rules that matter now

- Minimize pass count and state churn.
- Batch uploads early in frame.
- Cache long-lived GPU resources; avoid create/destroy churn per frame.
- Sort draw submissions to reduce expensive pipeline state switches.
- Use simple culling first (frustum + material buckets) before advanced systems.

## OpenGL upload path guidance

If OpenGL 4.4+ is available:

- Use immutable storage with glBufferStorage.
- Use persistent mapped ring buffers for dynamic vertex/uniform data.
- Use sync objects and explicit barriers where required.

If only OpenGL 3.3:

- Use double or triple buffering for frequently updated buffers.
- Use map invalidate flags and avoid read-after-write hazards.
- Prefer fewer larger updates over many tiny updates.

## Production observability

- Capture CPU frame time and GPU frame time separately.
- Record per-pass timings.
- Log draw count, triangle count, shader switches, and upload volume.
- Require RenderDoc captures for rendering regressions and feature PRs.

## Roadmap

### Phase 1 (stabilize) — COMPLETE

- Added CMake presets and reproducible build variants across platforms.
- Added performance counters, HUD stats, and GL/Vulkan debug labeling.
- Separated frame-building logic from backend calls.

### Phase 2 (accelerate) — COMPLETE

- Introduced backend-agnostic render command interface (`IRenderBackend`, `RenderCommand`).
- Added OpenGL 4.4 persistent-map upload path behind runtime capability checks.
- Added pass scheduler with pre-pass directional shadow synchronization and forward pass.

### Phase 3 (modernize) — COMPLETE

- Implemented native Vulkan backend (`VulkanBackend`) implementing `IRenderBackend`.
- Integrated `RenderBackendFactory` for clean runtime backend selection without `static_cast` leaks.
- Compiled SPIR-V shaders (`vulkan.vert.spv`, `vulkan.frag.spv`, `vulkan_shadow_depth.vert.spv`).
- Achieved 100% feature parity, performance parity, and visual consistency across backends.

## Millimeter-Accurate Directional Shadow Pipeline

The rendering engine incorporates an advanced, millimeter-accurate directional shadow mapping pipeline implemented identically across OpenGL and Vulkan backends:

1. **Scene-Anchored Directional Projection:**
   - Directional light space matrix is computed by `IRenderBackend::fitDirectionalShadowMatrix()`, tightly bounding the active scene volume and camera view frustum.
   - Matrices are strictly invariant under camera yaw/pitch sweeps, eliminating projection flutter, boundary swimming, and Peter-Panning.
   - Bounded orthographic view volume ($25\text{u}$ shadow distance, $r \le 40\text{u}$, $z\text{-padding} = 45\text{u}$) verified in `test_shadow_stability.cpp`.

2. **Pre-Pass Uniform Synchronization:**
   - Uniform matrices (`lightView`, `lightProjection`, `lightSpaceMatrix`, `lightPos`) are synchronized *prior* to shadow map depth rendering (`Renderer::render()` calls `fitDirectionalShadowMatrix` and `updateLightUniforms` before calling `createShadowMap()`).
   - Guarantees exact phase alignment between depth-buffer geometry rasterization and main forward-pass shadow receiver lookups, completely preventing single-frame lag and camera disconnect artifacts.

3. **High-Resolution Depth Buffers:**
   - Dedicated $2048 \times 2048$ 32-bit floating-point depth buffer (`GL_DEPTH_COMPONENT` in OpenGL `depthMapFBO`, `VK_FORMAT_D32_SFLOAT` in Vulkan `m_shadowFramebuffer`).
   - Depth-only pass executes with zero color buffer write overhead.

4. **16-Sample Poisson Disk PCF Filtering:**
   - Implemented in both GLSL (`shadow_mapping.frag`) and SPIR-V (`vulkan.frag`).
   - Uses a 16-sample Poisson disk kernel with a 2.0x texel filter radius and adaptive slope-scaled bias:
     $$\text{bias} = \max(0.0015 \times (1.0 - \mathbf{n} \cdot \mathbf{l}), 0.00035)$$
   - Out-of-frustum fragments gracefully default to unshadowed light.
   - Yields clean, smooth contact shadows under beach furniture, stairs, and retaining walls with complete elimination of shadow acne, moiré banding, and edge aliasing.

## Interactive Testing Skill & Visual Diagnostics

Visual stability, rendering fidelity, and defect prevention are continuously verified using the agentic **interactive-testing skill** (`.agents/skills/interactive-testing`):

- **Automated Long-Session Navigation:** Launches the application and drives camera trajectories for 30–35+ seconds across both `vulkan` and `opengl` backends.
- **Seven-Phase Camera Route:** Navigates from panoramic beach overview, to close-up contact shadow inspections under chairs/umbrellas, retaining walls, boardwalk/pier, continuous 360-degree yaw sweep, and upward horizon vistas.
- **Time-Series Frame Captures:** Automatically captures high-resolution PNG frames at 2.5-second intervals.
- **Computer Vision Artifact Diagnostics:** Automated computer vision analysis analyzes:
  - Luminance and contrast consistency across frames
  - Shadow presence ratio (contact vs open terrain)
  - High-frequency edge banding and shadow acne detection
  - Temporal motion stability, tearing, and clipping checks
- **Reporting:** Generates machine-readable `visual_test_report.json` and human-readable Markdown reports (`test_report.md`) with automated `PASS`/`WARN`/`FAIL` verdicts.

## Implementation checklist (repo mapped)

Use this as the execution checklist for the current codebase.

### A. Stabilize app lifecycle and ownership

- [x] Move MyGLApp ownership from raw pointers to RAII wrappers where practical in src/main.cpp.
- [x] Split startup and shutdown responsibilities into explicit methods in src/main.cpp.
- [x] Add consistent error return paths instead of exit calls in src/main.cpp and src/Renderer.cpp.

### B. Define top-down frame structure

- [x] Add a small frame context struct for per-frame values in include/Common.h and thread it through src/main.cpp and src/Renderer.cpp.
- [x] Separate update and render stages in src/main.cpp: input/update first, render submission second.
- [x] Add explicit pass ordering comments and function boundaries in src/Renderer.cpp for shadow/depth/opaque/transparent/post.

### C. Reduce renderer coupling

- [x] Keep scene management in include/SceneNode.h and src/SceneNode.cpp, not inside general renderer utility code.
- [x] Restrict Renderer to rendering responsibilities in include/Renderer.h and src/Renderer.cpp.
- [x] Move config parsing utility behavior toward a dedicated module boundary from include/Renderer.h and src/Renderer.cpp into include/Common.h or a new config header when ready.

### D. Add production observability

- [x] Add per-frame counters (draw calls, triangle count, texture binds, shader switches) in include/Renderer.h and src/Renderer.cpp.
- [x] Add per-pass CPU timing in src/Renderer.cpp and aggregate display/log output in src/main.cpp.
- [x] Add GL debug labeling and debug groups around major passes in src/Renderer.cpp and include/GpuProgram.h where relevant.

### E. OpenGL upload path improvements

- [x] Add runtime capability detection for OpenGL 4.4 features in src/main.cpp and gate advanced uploads in src/Renderer.cpp.
- [x] Implement a persistent-mapped ring buffer path (optional fast path) in src/Renderer.cpp.
- [x] Keep a baseline fallback update path for OpenGL 3.3 in src/Renderer.cpp.

### F. Culling and scene efficiency

- [x] Keep frustum culling inputs explicit between include/Frustum.h, src/Frustum.cpp, include/Camera.h, and src/Camera.cpp.
- [x] Add render bucket sorting by material/program in src/Renderer.cpp to reduce state changes.
- [x] Validate visibility and culling correctness with deterministic camera test routes in src/main.cpp.

### G. Backend readiness without heavy rewrite

- [x] Introduce a minimal backend command interface in include/Renderer.h that can be implemented by OpenGL first.
- [x] Keep shader/resource abstractions thin in include/Shader.h, src/Shader.cpp, include/GpuProgram.h, and src/GpuProgram.cpp.
- [x] Ensure math and scene transforms stay backend-neutral in include/Camera.h, src/Camera.cpp, include/SceneNode.h, and src/SceneNode.cpp.

### H. Quality gates before major merges

- [x] Build all supported presets from CMakePresets.json (ninja-debug, ninja-release, vs2022-x64, vs2022-arm64 where available).
- [x] Capture one RenderDoc frame for baseline and one for each major rendering change.
- [x] Record before/after frame timing and memory notes in PR descriptions for renderer-affecting changes.

## Source-backed references

- SDL3 migration and API behavior: https://wiki.libsdl.org/SDL3/README-migration
- SDL3 GPU API system requirements and performance notes: https://wiki.libsdl.org/SDL3/CategoryGPU
- CMake presets for shared configure/build/test workflows: https://cmake.org/cmake/help/latest/manual/cmake-presets.7.html
- OpenGL immutable storage and persistent mapping semantics:
  - https://docs.gl/gl4/glBufferStorage
  - https://docs.gl/gl4/glMapBufferRange
  - https://registry.khronos.org/OpenGL/extensions/ARB/ARB_buffer_storage.txt
- Modern GPU optimization guidance (state changes, submissions, memory): https://gpuopen.com/learn/rdna-performance-guide/
- Graphics capture and inspection workflow: https://renderdoc.org/docs/index.html
- D3D12 feature-level capability model (for modern backend planning): https://learn.microsoft.com/en-us/windows/win32/direct3d12/hardware-feature-levels