#!/usr/bin/env python3
"""
Interactive Visual Testing Framework for sdlgl3-wavefront
==========================================================
Executes an automated, camera-navigated visual testing session on a target 3D scene.
Controls camera movement via virtual inputs, captures visual frames over an extended duration
(>=30 seconds), evaluates images for visual defects (missing shadows, shadow acne, clipping,
stagnant frames), and produces a structured test report with artifacts.
"""

import argparse
import ctypes
from ctypes import wintypes
import json
import math
import os
import shutil
import subprocess
import sys
import time
from datetime import datetime
from pathlib import Path
from PIL import Image
import numpy as np

# Windows API constants
user32 = ctypes.windll.user32
gdi32 = ctypes.windll.gdi32

WM_KEYDOWN = 0x0100
WM_KEYUP = 0x0101
WM_CLOSE = 0x0010
VK_ESCAPE = 0x1B
VK_W = 0x57
VK_A = 0x41
VK_S = 0x53
VK_D = 0x44
VK_M = 0x4D
VK_SHIFT = 0x10
MOUSEEVENTF_MOVE = 0x0001
PW_RENDERFULLCONTENT = 2

class BITMAPINFOHEADER(ctypes.Structure):
    _fields_ = [
        ('biSize', wintypes.DWORD),
        ('biWidth', wintypes.LONG),
        ('biHeight', wintypes.LONG),
        ('biPlanes', wintypes.WORD),
        ('biBitCount', wintypes.WORD),
        ('biCompression', wintypes.DWORD),
        ('biSizeImage', wintypes.DWORD),
        ('biXPelsPerMeter', wintypes.LONG),
        ('biYPelsPerMeter', wintypes.LONG),
        ('biClrUsed', wintypes.DWORD),
        ('biClrImportant', wintypes.DWORD)
    ]

def find_app_window(pid: int, timeout: float = 15.0) -> wintypes.HWND:
    """Finds top-level window matching application process ID."""
    start = time.time()
    found_hwnd = None

    WNDENUMPROC = ctypes.WINFUNCTYPE(ctypes.c_bool, wintypes.HWND, wintypes.LPARAM)

    def enum_cb(hwnd, lparam):
        nonlocal found_hwnd
        p = wintypes.DWORD()
        user32.GetWindowThreadProcessId(hwnd, ctypes.byref(p))
        if p.value == pid:
            length = user32.GetWindowTextLengthW(hwnd)
            buf = ctypes.create_unicode_buffer(length + 1)
            user32.GetWindowTextW(hwnd, buf, length + 1)
            title = buf.value.lower()
            if 'sdlgl' in title or 'wavefront' in title:
                found_hwnd = hwnd
                return False
        return True

    cb = WNDENUMPROC(enum_cb)
    while time.time() - start < timeout:
        user32.EnumWindows(cb, 0)
        if found_hwnd:
            return found_hwnd
        time.sleep(0.2)
    return None

def capture_window_image(hwnd: wintypes.HWND) -> Image.Image:
    """Captures the contents of a window using Windows PrintWindow with PW_RENDERFULLCONTENT."""
    rect = wintypes.RECT()
    user32.GetClientRect(hwnd, ctypes.byref(rect))
    w = max(rect.right - rect.left, 1)
    h = max(rect.bottom - rect.top, 1)

    hdc_screen = user32.GetDC(0)
    hdc_mem = gdi32.CreateCompatibleDC(hdc_screen)
    hbm = gdi32.CreateCompatibleBitmap(hdc_screen, w, h)
    gdi32.SelectObject(hdc_mem, hbm)

    ret = user32.PrintWindow(hwnd, hdc_mem, PW_RENDERFULLCONTENT)
    if not ret:
        user32.PrintWindow(hwnd, hdc_mem, 0)

    bmi = BITMAPINFOHEADER()
    bmi.biSize = ctypes.sizeof(BITMAPINFOHEADER)
    bmi.biWidth = w
    bmi.biHeight = -h
    bmi.biPlanes = 1
    bmi.biBitCount = 32
    bmi.biCompression = 0

    buf = ctypes.create_string_buffer(w * h * 4)
    gdi32.GetDIBits(hdc_mem, hbm, 0, h, buf, ctypes.byref(bmi), 0)
    img = Image.frombuffer('RGBA', (w, h), buf, 'raw', 'BGRA', 0, 1).convert('RGB')

    gdi32.DeleteObject(hbm)
    gdi32.DeleteDC(hdc_mem)
    user32.ReleaseDC(0, hdc_screen)
    return img

def analyze_visual_frame(img: Image.Image, prev_img: Image.Image = None) -> dict:
    """Analyzes a captured visual frame for quality, shadow presence, and artifacts."""
    arr = np.array(img, dtype=np.float32) / 255.0
    r, g, b = arr[:, :, 0], arr[:, :, 1], arr[:, :, 2]
    luminance = 0.299 * r + 0.587 * g + 0.114 * b

    mean_lum = float(np.mean(luminance))
    min_lum = float(np.min(luminance))
    max_lum = float(np.max(luminance))
    lum_std = float(np.std(luminance))

    # Shadow presence: ratio of shaded pixels (in reasonable range [0.05, 0.45]) vs bright lit surfaces (> 0.60)
    shadow_pixels = np.sum((luminance >= 0.05) & (luminance <= 0.45))
    lit_pixels = np.sum(luminance > 0.60)
    shadow_ratio = float(shadow_pixels) / float(max(lit_pixels, 1))

    # Shadow acne / high frequency horizontal banding detection:
    # Acne typically manifests as alternating bright/dark horizontal scanline bands
    row_means = np.mean(luminance, axis=1)
    row_diffs = np.abs(np.diff(row_means))
    high_freq_banding_score = float(np.std(row_diffs))

    # Frame motion detection: comparison with previous frame
    motion_fraction = 0.0
    mean_abs_diff = 0.0
    if prev_img is not None:
        prev_arr = np.array(prev_img, dtype=np.float32) / 255.0
        diff = np.abs(arr - prev_arr)
        motion_fraction = float(np.mean(diff > 0.05))
        mean_abs_diff = float(np.mean(diff))

    is_black_screen = (max_lum < 0.05)
    is_white_screen = (min_lum > 0.95)
    is_acne_suspect = (high_freq_banding_score > 0.08)

    return {
        "mean_luminance": round(mean_lum, 4),
        "min_luminance": round(min_lum, 4),
        "max_luminance": round(max_lum, 4),
        "luminance_std": round(lum_std, 4),
        "shadow_pixels_ratio": round(shadow_ratio, 4),
        "banding_score": round(high_freq_banding_score, 5),
        "motion_fraction": round(motion_fraction, 4),
        "mean_pixel_diff": round(mean_abs_diff, 4),
        "defect_black_screen": is_black_screen,
        "defect_white_screen": is_white_screen,
        "defect_shadow_acne_suspect": is_acne_suspect
    }

def update_config_file(config_path: Path, backend: str, close_on_load: bool = False):
    """Updates config/app.cfg with specified backend and settings."""
    if not config_path.exists():
        return
    lines = config_path.read_text(encoding='utf-8').splitlines()
    new_lines = []
    has_backend = False
    has_close = False
    for line in lines:
        if line.strip().startswith('renderer.backend'):
            new_lines.append(f"renderer.backend = {backend}")
            has_backend = True
        elif line.strip().startswith('closeOnLoad'):
            new_lines.append(f"closeOnLoad = {'true' if close_on_load else 'false'}")
            has_close = True
        elif line.strip().startswith('camera.position') or line.strip().startswith('camera.target'):
            continue  # remove position overrides to allow dynamic navigation
        else:
            new_lines.append(line)
    if not has_backend:
        new_lines.append(f"renderer.backend = {backend}")
    if not has_close:
        new_lines.append(f"closeOnLoad = {'true' if close_on_load else 'false'}")
    config_path.write_text('\n'.join(new_lines) + '\n', encoding='utf-8')

def run_interactive_test(model: str = "models/scene.obj",
                         backend: str = "vulkan",
                         duration: float = 35.0,
                         interval: float = 2.5,
                         output_dir: Path = None,
                         build: bool = True) -> int:
    """Runs a full interactive visual test session."""
    workspace = Path.cwd()
    if output_dir is None:
        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        output_dir = workspace / "artifacts" / f"visual_test_{backend}_{timestamp}"
    output_dir.mkdir(parents=True, exist_ok=True)

    print(f"=== Starting Interactive Visual Test ===")
    print(f"Model: {model}")
    print(f"Backend: {backend}")
    print(f"Duration: {duration:.1f}s (capture interval: {interval:.1f}s)")
    print(f"Output Directory: {output_dir}")

    # Build step
    if build:
        print("\n[1/5] Building project (Debug)...")
        build_cmd = ["cmake", "--build", "build", "--config", "Debug"]
        res = subprocess.run(build_cmd, capture_output=True, text=True)
        if res.returncode != 0:
            print(f"Build failed:\n{res.stderr}\n{res.stdout}")
            return 1
        print("Build succeeded.")

    # Configure app.cfg
    config_path = workspace / "config" / "app.cfg"
    orig_config = config_path.read_text(encoding='utf-8') if config_path.exists() else None
    update_config_file(config_path, backend=backend, close_on_load=False)

    exe_path = workspace / "build" / "Debug" / "sdlglapp.exe"
    if not exe_path.exists():
        exe_path = workspace / "build" / "sdlglapp.exe"
    if not exe_path.exists():
        print(f"Executable not found at {exe_path}")
        return 1

    proc = None
    try:
        print("\n[2/5] Launching sdlglapp process...")
        cmd = [str(exe_path), model]
        proc = subprocess.Popen(cmd, cwd=str(workspace), stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
        print(f"Process spawned with PID {proc.pid}. Waiting for window initialization...")

        hwnd = find_app_window(proc.pid, timeout=12.0)
        if not hwnd:
            print("Failed to locate application window.")
            return 1

        print(f"Window located (HWND: {hwnd}). Bringing window into foreground...")
        user32.SetForegroundWindow(hwnd)
        time.sleep(2.0)  # Wait for scene graph and atlas buffer upload

        print("\n[3/5] Executing dynamic camera navigation & capture loop (30s+)...")
        start_time = time.time()
        last_capture_time = 0.0
        frame_idx = 0
        captures_data = []
        prev_img = None

        # Key navigation phases over 35 seconds:
        # Phase 1 (0-4s): Initial perspective (overview)
        # Phase 2 (4-9s): Advance towards beach & chairs (Key W)
        # Phase 3 (9-14s): Pitch down and inspect umbrella & table contact shadows
        # Phase 4 (14-19s): Turn right towards retaining wall + strafe left (Key A + Mouse Right)
        # Phase 5 (19-24s): Strafe towards pier (Key D + Mouse Left)
        # Phase 6 (24-29s): 360-degree pan sweep to verify shadow matrix stability
        # Phase 7 (29-35s): Tilt up to beach house roof & shoreline overview

        active_key = None

        while True:
            elapsed = time.time() - start_time
            if elapsed >= duration:
                break

            # Steer camera trajectory based on timeline
            if 4.0 <= elapsed < 9.0:
                if active_key != VK_W:
                    if active_key: user32.PostMessageW(hwnd, WM_KEYUP, active_key, 0)
                    active_key = VK_W
                    user32.PostMessageW(hwnd, WM_KEYDOWN, active_key, 0)
            elif 9.0 <= elapsed < 14.0:
                if active_key:
                    user32.PostMessageW(hwnd, WM_KEYUP, active_key, 0)
                    active_key = None
                user32.mouse_event(MOUSEEVENTF_MOVE, 0, 8, 0, 0)  # aim down
            elif 14.0 <= elapsed < 19.0:
                if active_key != VK_A:
                    if active_key: user32.PostMessageW(hwnd, WM_KEYUP, active_key, 0)
                    active_key = VK_A
                    user32.PostMessageW(hwnd, WM_KEYDOWN, active_key, 0)
                user32.mouse_event(MOUSEEVENTF_MOVE, 12, -2, 0, 0)  # turn right
            elif 19.0 <= elapsed < 24.0:
                if active_key != VK_D:
                    if active_key: user32.PostMessageW(hwnd, WM_KEYUP, active_key, 0)
                    active_key = VK_D
                    user32.PostMessageW(hwnd, WM_KEYDOWN, active_key, 0)
                user32.mouse_event(MOUSEEVENTF_MOVE, -15, 0, 0)  # turn left
            elif 24.0 <= elapsed < 29.0:
                if active_key:
                    user32.PostMessageW(hwnd, WM_KEYUP, active_key, 0)
                    active_key = None
                user32.mouse_event(MOUSEEVENTF_MOVE, 25, 0, 0)  # 360 yaw sweep
            elif 29.0 <= elapsed < 35.0:
                if active_key != VK_S:
                    if active_key: user32.PostMessageW(hwnd, WM_KEYUP, active_key, 0)
                    active_key = VK_S
                    user32.PostMessageW(hwnd, WM_KEYDOWN, active_key, 0)
                user32.mouse_event(MOUSEEVENTF_MOVE, 0, -10, 0, 0)  # tilt up

            # Periodic visual capture
            if elapsed - last_capture_time >= interval:
                frame_idx += 1
                cap_filename = f"frame_{frame_idx:03d}_{elapsed:04.1f}s.png"
                cap_path = output_dir / cap_filename
                img = capture_window_image(hwnd)
                img.save(cap_path)

                # Send M key for engine internal capture if supported
                user32.PostMessageW(hwnd, WM_KEYDOWN, VK_M, 0)
                user32.PostMessageW(hwnd, WM_KEYUP, VK_M, 0)

                # Analyze frame
                metrics = analyze_visual_frame(img, prev_img)
                prev_img = img
                last_capture_time = elapsed

                frame_record = {
                    "frame": frame_idx,
                    "timestamp": round(elapsed, 2),
                    "filename": cap_filename,
                    "path": str(cap_path),
                    "metrics": metrics
                }
                captures_data.append(frame_record)
                print(f"  [Capture {frame_idx:02d} @ {elapsed:04.1f}s] {cap_filename} | "
                      f"Lum: {metrics['mean_luminance']:.2f} | ShdRatio: {metrics['shadow_pixels_ratio']:.2f} | "
                      f"Banding: {metrics['banding_score']:.4f} | Motion: {metrics['motion_fraction']:.2f}")

            time.sleep(0.08)

        # Release any held keys
        if active_key:
            user32.PostMessageW(hwnd, WM_KEYUP, active_key, 0)

        print("\n[4/5] Concluding session and closing application...")
        user32.PostMessageW(hwnd, WM_KEYDOWN, VK_ESCAPE, 0)
        try:
            proc.wait(timeout=5.0)
            print("Process terminated gracefully.")
        except subprocess.TimeoutExpired:
            print("Process did not exit on ESC; terminating...")
            proc.terminate()

        # Generate Report
        print("\n[5/5] Generating Visual Quality and Defect Analysis Report...")
        total_frames = len(captures_data)
        black_frames = sum(1 for c in captures_data if c["metrics"]["defect_black_screen"])
        white_frames = sum(1 for c in captures_data if c["metrics"]["defect_white_screen"])
        acne_frames = sum(1 for c in captures_data if c["metrics"]["defect_shadow_acne_suspect"])
        moving_frames = sum(1 for c in captures_data if c["metrics"]["motion_fraction"] > 0.02)

        mean_shd_ratio = float(np.mean([c["metrics"]["shadow_pixels_ratio"] for c in captures_data])) if captures_data else 0.0
        mean_banding = float(np.mean([c["metrics"]["banding_score"] for c in captures_data])) if captures_data else 0.0

        verdict = "PASS"
        issues = []
        if black_frames > 0:
            verdict = "FAIL"
            issues.append(f"{black_frames} black/blank frames detected")
        if white_frames > 0:
            verdict = "FAIL"
            issues.append(f"{white_frames} overexposed/white frames detected")
        if mean_shd_ratio < 0.05:
            verdict = "FAIL"
            issues.append("Zero/minimal shadow presence detected across entire session")
        if acne_frames > total_frames * 0.3:
            verdict = "WARN"
            issues.append(f"Potential shadow acne / stripe banding in {acne_frames} frames")
        if moving_frames < (total_frames - 2):
            issues.append("Camera movement was partially static")

        report = {
            "test_timestamp": datetime.now().isoformat(),
            "model": model,
            "backend": backend,
            "duration_seconds": round(duration, 1),
            "total_frames_captured": total_frames,
            "verdict": verdict,
            "issues": issues,
            "summary_metrics": {
                "average_shadow_ratio": round(mean_shd_ratio, 4),
                "average_banding_score": round(mean_banding, 5),
                "moving_frames_count": moving_frames
            },
            "frames": captures_data
        }

        # Write JSON
        report_json_path = output_dir / "visual_test_report.json"
        with open(report_json_path, "w", encoding="utf-8") as f:
            json.dump(report, f, indent=2)

        # Write Markdown
        report_md_path = output_dir / "test_report.md"
        with open(report_md_path, "w", encoding="utf-8") as f:
            f.write(f"# Interactive Visual Test Report\n\n")
            f.write(f"- **Date**: {report['test_timestamp']}\n")
            f.write(f"- **Model**: `{model}`\n")
            f.write(f"- **Backend**: `{backend}`\n")
            f.write(f"- **Duration**: {duration:.1f} seconds\n")
            f.write(f"- **Frames Captured**: {total_frames}\n")
            f.write(f"- **Verdict**: **{verdict}**\n\n")

            if issues:
                f.write(f"### Detected Issues\n")
                for issue in issues:
                    f.write(f"- {issue}\n")
                f.write("\n")
            else:
                f.write("### Detected Issues\n- None. Visuals are clean, stable, and correctly shadowed.\n\n")

            f.write("### Captured Frames\n\n")
            f.write("| Frame | Time | Filename | Mean Lum | Shadow Ratio | Banding | Motion |\n")
            f.write("|-------|------|----------|----------|--------------|---------|--------|\n")
            for c in captures_data:
                m = c["metrics"]
                f.write(f"| {c['frame']} | {c['timestamp']}s | [{c['filename']}]({c['filename']}) | "
                        f"{m['mean_luminance']:.2f} | {m['shadow_pixels_ratio']:.2f} | {m['banding_score']:.4f} | {m['motion_fraction']:.2f} |\n")

        print(f"\n==========================================")
        print(f"Test Complete! Verdict: {verdict}")
        print(f"Report: {report_md_path}")
        print(f"Data: {report_json_path}")
        print(f"==========================================")
        return 0 if verdict in ("PASS", "WARN") else 1

    finally:
        if orig_config is not None:
            config_path.write_text(orig_config, encoding='utf-8')
        if proc and proc.poll() is None:
            proc.terminate()

def main():
    parser = argparse.ArgumentParser(description="Interactive visual testing framework for sdlgl3-wavefront")
    parser.add_argument("--model", default="models/scene.obj", help="Path to 3D scene model")
    parser.add_argument("--backend", default="vulkan", choices=["vulkan", "opengl"], help="Rendering backend")
    parser.add_argument("--duration", type=float, default=35.0, help="Test duration in seconds (>=30s)")
    parser.add_argument("--interval", type=float, default=2.5, help="Capture interval in seconds")
    parser.add_argument("--output-dir", default=None, help="Directory to save captures and reports")
    parser.add_argument("--no-build", action="store_true", help="Skip project build step")
    args = parser.parse_args()

    ret = run_interactive_test(
        model=args.model,
        backend=args.backend,
        duration=args.duration,
        interval=args.interval,
        output_dir=Path(args.output_dir) if args.output_dir else None,
        build=not args.no_build
    )
    sys.exit(ret)

if __name__ == "__main__":
    main()
