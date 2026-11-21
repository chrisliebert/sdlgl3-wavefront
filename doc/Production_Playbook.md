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
   - Backend-agnostic command model.
5. Backend Layer
   - OpenGL backend (today).
   - SDL GPU API backend (next, maps to Vulkan/D3D12/Metal).

Keep dependencies one-way (top to bottom only).

## Hardware strategy

### Tier A: Compatibility (required)

- API: OpenGL 3.3 core.
- Goal: broad desktop coverage and stable correctness.
- Constraints: no persistent mapping requirement, no bindless assumptions.

### Tier B: OpenGL fast path (optional)

- API target: OpenGL 4.4+ capabilities when present.
- Use immutable buffers and persistent mapping for dynamic uploads.
- Keep fallback path using orphaning and map/unmap.

### Tier C: Modern backend (optional)

- API: SDL3 GPU API to target D3D12, Vulkan, and Metal from one abstraction.
- Goal: access modern queue/pipeline behavior without per-platform forked renderer.

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

### Phase 1 (stabilize)

- Add CMake presets and reproducible build variants.
- Add performance counters and debug labeling.
- Separate frame-building logic from backend calls.

### Phase 2 (accelerate)

- Introduce backend-agnostic render command interface.
- Add OpenGL 4.4 persistent-map upload path behind runtime capability checks.
- Add pass scheduler (shadow and opaque first).

### Phase 3 (modernize)

- Add SDL GPU backend prototype for one scene path.
- Keep OpenGL backend as compatibility path.
- Compare feature parity and performance across backends.

## Implementation checklist (repo mapped)

Use this as the execution checklist for the current codebase.

### A. Stabilize app lifecycle and ownership

- [ ] Move MyGLApp ownership from raw pointers to RAII wrappers where practical in src/main.cpp.
- [ ] Split startup and shutdown responsibilities into explicit methods in src/main.cpp.
- [ ] Add consistent error return paths instead of exit calls in src/main.cpp and src/Renderer.cpp.

### B. Define top-down frame structure

- [ ] Add a small frame context struct for per-frame values in include/Common.h and thread it through src/main.cpp and src/Renderer.cpp.
- [ ] Separate update and render stages in src/main.cpp: input/update first, render submission second.
- [ ] Add explicit pass ordering comments and function boundaries in src/Renderer.cpp for shadow/depth/opaque/transparent/post.

### C. Reduce renderer coupling

- [ ] Keep scene management in include/SceneNode.h and src/SceneNode.cpp, not inside general renderer utility code.
- [ ] Restrict Renderer to rendering responsibilities in include/Renderer.h and src/Renderer.cpp.
- [ ] Move config parsing utility behavior toward a dedicated module boundary from include/Renderer.h and src/Renderer.cpp into include/Common.h or a new config header when ready.

### D. Add production observability

- [ ] Add per-frame counters (draw calls, triangle count, texture binds, shader switches) in include/Renderer.h and src/Renderer.cpp.
- [ ] Add per-pass CPU timing in src/Renderer.cpp and aggregate display/log output in src/main.cpp.
- [ ] Add GL debug labeling and debug groups around major passes in src/Renderer.cpp and include/GpuProgram.h where relevant.

### E. OpenGL upload path improvements

- [ ] Add runtime capability detection for OpenGL 4.4 features in src/main.cpp and gate advanced uploads in src/Renderer.cpp.
- [ ] Implement a persistent-mapped ring buffer path (optional fast path) in src/Renderer.cpp.
- [ ] Keep a baseline fallback update path for OpenGL 3.3 in src/Renderer.cpp.

### F. Culling and scene efficiency

- [ ] Keep frustum culling inputs explicit between include/Frustum.h, src/Frustum.cpp, include/Camera.h, and src/Camera.cpp.
- [ ] Add render bucket sorting by material/program in src/Renderer.cpp to reduce state changes.
- [ ] Validate visibility and culling correctness with deterministic camera test routes in src/main.cpp.

### G. Backend readiness without heavy rewrite

- [ ] Introduce a minimal backend command interface in include/Renderer.h that can be implemented by OpenGL first.
- [ ] Keep shader/resource abstractions thin in include/Shader.h, src/Shader.cpp, include/GpuProgram.h, and src/GpuProgram.cpp.
- [ ] Ensure math and scene transforms stay backend-neutral in include/Camera.h, src/Camera.cpp, include/SceneNode.h, and src/SceneNode.cpp.

### H. Quality gates before major merges

- [ ] Build all supported presets from CMakePresets.json (ninja-debug, ninja-release, vs2022-x64, vs2022-arm64 where available).
- [ ] Capture one RenderDoc frame for baseline and one for each major rendering change.
- [ ] Record before/after frame timing and memory notes in PR descriptions for renderer-affecting changes.

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