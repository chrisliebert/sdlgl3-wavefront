# Rendering Optimization Research (2026-09-06)

## Scope
This note summarizes implemented optimization work and next high-impact rendering optimizations for this repository.

## Implemented now: Diffuse texture atlas optimization

### What was added
- Config toggles:
  - `renderer.textureAtlas.enabled`
  - `renderer.textureAtlas.maxSize`
  - `renderer.textureAtlas.padding`
- Runtime atlas builder that:
  - collects packable diffuse textures
  - packs into a single atlas (shelf pack)
  - remaps node UVs for packed textures
  - uploads one atlas texture and rewrites packed node diffuse IDs
- Safety behavior:
  - partial packing if some textures are too large
  - CPU vertex data is restored after GPU upload to preserve cache semantics

### Implementation references
- Atlas config load: `src/Renderer.cpp:42`
- Texture key tracking per node: `src/Renderer.cpp:1212`
- Atlas build/remap: `src/Renderer.cpp:1280`
- Atlas integration in upload path: `src/Renderer.cpp:1500`
- Atlas config values: `config/renderer.cfg:7`

### Observed runtime signal
- On `models/scene.obj`, atlas now activates with log similar to:
  - `Texture atlas built: 13 textures -> 4004x4033, remapped nodes=9`

## Research findings: next optimization opportunities

## P0 (high impact, low/medium risk)

### 1) Vulkan descriptor/sampler deduplication cache
Current Vulkan texture upload creates image view + sampler + descriptor set per texture instance (`src/VulkanBackend.cpp:584`, `src/VulkanBackend.cpp:602`, `src/VulkanBackend.cpp:619`).

Recommendation:
- Cache immutable samplers globally (usually one linear wrap sampler).
- Cache descriptor sets by `(imageView, sampler)` key.
- Reuse descriptor sets across materials that share the same texture.

Expected impact:
- Lower CPU overhead and descriptor pool churn during scene load/reload.

### 2) Generate mipmaps for Vulkan diffuse textures
Current Vulkan path uploads single-level textures (`mipLevels = 1` at `src/VulkanBackend.cpp:519`).

Recommendation:
- Generate mip chains at upload time.
- Sample with mip filtering.

Expected impact:
- Better texture quality and less shimmer/aliasing in distance.
- Better texture cache behavior on GPU.

### 3) Atlas-aware cache metadata
Current cache stores original texture inventory and geometry; atlas is rebuilt each run.

Recommendation:
- Add optional cache chunk for atlas metadata (packed keys + UV transforms).
- Keep source geometry cache unchanged; apply cached transforms at upload time.

Expected impact:
- Faster startup after first atlas build.

## P1 (high impact, medium risk)

### 4) Vulkan queue-family and present support selection
Current logical device path assumes queue family index 0 (`src/VulkanBackend.cpp:689`).

Recommendation:
- Probe graphics and present families explicitly.
- Use dedicated transfer queue if available for uploads.

Expected impact:
- Better portability/stability across drivers.
- Potentially better upload concurrency.

### 5) Pipeline state specialization for culling and shadow quality
Renderer already has useful toggles (`config/renderer.cfg:14` onward), but Vulkan uses one fixed pipeline.

Recommendation:
- Build two pipeline variants: cull off/on.
- Optional shadow-quality profile presets (resolution, PCF taps).

Expected impact:
- Better runtime tuning for low-end versus high-end devices.

## P2 (medium impact, low risk)

### 6) Improve visibility scheduling
Renderer performs sort and cull each frame (`src/Renderer.cpp:1835`, `src/Renderer.cpp:1630`).

Recommendation:
- Keep temporal coherence buckets by camera cell.
- Skip full re-sort if visible set changed below threshold.

Expected impact:
- Lower CPU frame cost in camera-stable shots.

### 7) Async texture decode + upload staging
Recommendation:
- Decode images off-thread.
- Upload in batched staging windows on render thread.

Expected impact:
- Smoother scene load and reduced frame spikes.

## Suggested implementation order
1. Vulkan descriptor/sampler dedup cache.
2. Vulkan mipmap generation.
3. Atlas metadata cache chunk.
4. Queue-family selection improvements.
5. Visibility scheduling refinements.

## Validation plan
- Use existing `renderer.profileHud` to compare CPU frame time and draw behavior.
- Add startup timing logs around texture load and upload.
- Run A/B on:
  - atlas enabled/disabled
  - mipmaps enabled/disabled
  - descriptor cache enabled/disabled
