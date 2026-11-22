#!/usr/bin/env python3
"""
Visual Defect Analyzer for Captured Frames
===========================================
Performs post-hoc computer vision and statistical analysis on captured rendering frames.
Evaluates:
- Blank/black/white screen detection
- Shadow presence and contrast distribution
- High-frequency moiré / shadow acne detection
- Inter-frame temporal stability and geometry culling pop-in
"""

import argparse
import json
from pathlib import Path
from PIL import Image
import numpy as np

def analyze_directory(capture_dir: Path) -> dict:
    images = sorted(list(capture_dir.glob("*.png")) + list(capture_dir.glob("*.ppm")))
    if not images:
        print(f"No PNG or PPM frames found in {capture_dir}")
        return {"status": "error", "message": "No frames found"}

    print(f"Analyzing {len(images)} frames in {capture_dir}...")
    results = []
    prev_arr = None

    for idx, img_path in enumerate(images):
        img = Image.open(img_path).convert('RGB')
        arr = np.array(img, dtype=np.float32) / 255.0
        r, g, b = arr[:, :, 0], arr[:, :, 1], arr[:, :, 2]
        lum = 0.299 * r + 0.587 * g + 0.114 * b

        mean_lum = float(np.mean(lum))
        min_lum = float(np.min(lum))
        max_lum = float(np.max(lum))
        lum_std = float(np.std(lum))

        shadow_pixels = np.sum((lum >= 0.05) & (lum <= 0.45))
        lit_pixels = np.sum(lum > 0.60)
        shadow_ratio = float(shadow_pixels) / float(max(lit_pixels, 1))

        row_means = np.mean(lum, axis=1)
        row_diffs = np.abs(np.diff(row_means))
        banding_score = float(np.std(row_diffs))

        diff_from_prev = 0.0
        if prev_arr is not None:
            diff_from_prev = float(np.mean(np.abs(arr - prev_arr)))
        prev_arr = arr

        results.append({
            "index": idx + 1,
            "filename": img_path.name,
            "mean_luminance": round(mean_lum, 4),
            "dynamic_range": round(max_lum - min_lum, 4),
            "shadow_ratio": round(shadow_ratio, 4),
            "banding_score": round(banding_score, 5),
            "frame_delta": round(diff_from_prev, 4),
            "defect_black": bool(max_lum < 0.05),
            "defect_white": bool(min_lum > 0.95),
            "defect_acne_suspect": bool(banding_score > 0.08)
        })

    summary = {
        "directory": str(capture_dir),
        "total_frames": len(images),
        "frames": results
    }

    report_path = capture_dir / "defect_analysis.json"
    with open(report_path, "w", encoding="utf-8") as f:
        json.dump(summary, f, indent=2)

    print(f"Analysis saved to {report_path}")
    return summary

def main():
    parser = argparse.ArgumentParser(description="Analyze visual frames for rendering defects")
    parser.add_argument("directory", help="Directory containing captured frames")
    args = parser.parse_args()
    analyze_directory(Path(args.directory))

if __name__ == "__main__":
    main()
