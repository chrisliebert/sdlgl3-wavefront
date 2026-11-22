# Visual Defect Catalog & Rubric

This catalog outlines the primary visual defects encountered in real-time 3D rendering with OpenGL and Vulkan, along with diagnostic signatures and automated detection criteria.

---

## 1. Missing Shadows (Depth Map Underflow / Uniform Mismatch)
- **Visual Appearance**: Scene appears flat and overly bright. Under-table, chair, and umbrella regions show no contact shadows on the ground surface.
- **Root Cause**: Shadow pass executed with uninitialized/identity matrix or shadow depth map cleared but never populated; sampler reading cleared 1.0 depth.
- **Metric Signature**: `shadow_pixels_ratio < 0.05` across brightly lit scenes.

---

## 2. Shadow Acne & Moiré Banding (Depth Bias Insufficiency)
- **Visual Appearance**: Zebra stripes, horizontal scanline banding, or concentric ring artifacts across planar surfaces (e.g. beach sand, walls).
- **Root Cause**: Floating-point precision quantization between camera view depth and shadow map depth; surface self-shadowing due to inadequate constant or slope-scaled bias.
- **Metric Signature**: High standard deviation of inter-row luminance diffs (`banding_score > 0.08`).

---

## 3. Shadow Swimming & Shimmer (Camera-Coupled Projection)
- **Visual Appearance**: Shadow edges crawl, jitter, or drift across the terrain when the camera rotates or translates, despite static light sources and geometry.
- **Root Cause**: Directional shadow matrix bounds or center dynamically recalculating based on camera position rather than fixed scene volume bounds.
- **Metric Signature**: Large high-frequency edge shifts during translation without light movement.

---

## 4. Frustum Culling Pop-in / Disappearing Geometry
- **Visual Appearance**: Large scene objects (e.g. pier, house roof, umbrellas) suddenly disappear when looking slightly away from their center.
- **Root Cause**: Bounding sphere radius too small, failure to transform bounding sphere into world space, or incorrect plane extraction in view-projection frustum.
- **Metric Signature**: Sudden drop in mean luminance or large localized frame delta when camera slowly pans across the boundary.

---

## 5. Blank / Black / White Screen (Pipeline Stalls or Clear-Only)
- **Visual Appearance**: Window is solid black or solid white.
- **Root Cause**: Vertex clipping discarding all primitives; swapchain acquisition failure; shader compilation or descriptor binding failure.
- **Metric Signature**: `max_luminance < 0.05` (Black) or `min_luminance > 0.95` (White).
