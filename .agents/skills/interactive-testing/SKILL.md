---
name: interactive-testing
description: >-
  Build, run, and visually test sdlgl3-wavefront scenes interactively.
  Launches the application, navigates the camera through dynamic trajectories,
  captures high-fidelity visual frames over extended sessions (30s+),
  and runs automated computer vision analysis for visual artifacts
  (shadow acne, missing shadows, tearing, clipping, edge contrast).
---

# Interactive Visual Testing Skill

This skill provides an automated, agentic framework for verifying the visual rendering quality of `sdlgl3-wavefront`. It executes interactive, camera-navigated rendering sessions for 30+ seconds, captures time-series visual frames, and inspects them for defects.

## Overview & Capabilities

- **Automated Compilation**: Ensures binaries and SPIR-V shaders are clean before test runs.
- **Multi-Backend Testing**: Supports both Vulkan (`vulkan`) and OpenGL (`opengl`).
- **Dynamic Camera Trajectory**: Drives the camera via WASD movement and mouse look across diverse viewpoints (beach overview, close-up contact shadows, retaining wall, boardwalk/pier, 360-degree yaw rotation).
- **Time-Series Captures**: Samples high-resolution frames every 2.5 seconds over 30+ seconds.
- **Automated Defect Diagnostics**: Analyzes luminance, shadow presence ratio, acne/banding frequency, and temporal motion to flag rendering defects.

---

## Running an Interactive Test

To run a 35-second visual test session on `models/scene.obj` with Vulkan:

```powershell
python .agents/skills/interactive-testing/scripts/interactive_visual_test.py --backend vulkan --duration 35.0 --interval 2.5
```

To run with OpenGL:

```powershell
python .agents/skills/interactive-testing/scripts/interactive_visual_test.py --backend opengl --duration 35.0 --interval 2.5
```

### Script Arguments

| Flag | Default | Description |
|------|---------|-------------|
| `--model` | `models/scene.obj` | 3D scene model to load and test |
| `--backend` | `vulkan` | Rendering backend (`vulkan` or `opengl`) |
| `--duration` | `35.0` | Test run duration in seconds (must be $\ge 30\text{s}$) |
| `--interval` | `2.5` | Sampling period in seconds between frame captures |
| `--output-dir` | Auto-generated | Path to directory where PNG frames and reports are saved |
| `--no-build` | False | Skips the compilation phase before launching |

---

## Camera Trajectory Phases (35s Session)

1. **Phase 1 ($0\text{s} - 4\text{s}$)**: Scene load, asset decompression, and initial panoramic overview.
2. **Phase 2 ($4\text{s} - 9\text{s}$)**: Advance towards the beach chairs and umbrella (`Key W`).
3. **Phase 3 ($9\text{s} - 14\text{s}$)**: Pitch camera downward to inspect table, umbrella, and chair contact shadows on the sand.
4. **Phase 4 ($14\text{s} - 19\text{s}$)**: Turn right towards the retaining wall and stairs (`Key A` + Mouse Right).
5. **Phase 5 ($19\text{s} - 24\text{s}$)**: Strafe toward the wooden pier and boardwalk (`Key D` + Mouse Left).
6. **Phase 6 ($24\text{s} - 29\text{s}$)**: Execute continuous 360-degree yaw sweep to evaluate shadow matrix stability and frustum culling.
7. **Phase 7 ($29\text{s} - 35\text{s}$)**: Tilt upward towards beach house roof and ocean horizon overview (`Key S` + Mouse Up).

---

## Output Artifacts & Diagnostics

Every test session produces:
- `frame_XXX_YY.Ys.png`: High-resolution visual snapshots at each time step.
- `visual_test_report.json`: Machine-readable metrics for each frame.
- `test_report.md`: Human-readable summary table with verdict (`PASS`, `WARN`, `FAIL`).

Refer to the [Visual Defect Catalog](./references/defect_catalog.md) for full descriptions of defect classifications and thresholds.
