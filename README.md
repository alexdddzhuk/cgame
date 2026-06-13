# cgame — 3D OpenGL Cube

A GPU-accelerated 3D OpenGL app written in C++ that renders a textured, lit cube with a full ImGui settings panel.

## Controls

| Input | Action |
|---|---|
| `↑ ↓ ← →` | Rotate cube |
| `LMB drag` | Free rotate (when auto-rotate is off) |
| `Scroll wheel` | Zoom in / out |
| `Alt+Enter` | Toggle fullscreen |
| `ESC` | Quit |
| `☰` button (top-left) | Open / close settings panel |

## Settings Panel

| Section | Options |
|---|---|
| **Rotation** | Pitch / Yaw sliders, Reset button |
| **Auto-Rotate** | D3D-style tumble, configurable X/Y speed; when off — mouse drag + inertia spin |
| **Display** | Fullscreen toggle, Wireframe, Lighting, Scale, Arrow speed, Mouse sensitivity, Background color |
| **Texture** | Browse any PNG/JPG/BMP/TGA, Use as bump map, Bump strength slider |
| **Lighting** | Light direction, Light color, Specular strength, Shininess (Blinn-Phong) |
| **Camera** | Distance slider + scroll wheel zoom |

## Features

- **Blinn-Phong lighting** — ambient + diffuse + specular highlights
- **Bump mapping** — derives perturbed normals from the texture's luminance gradient (tangent-space TBN reconstruction)
- **Mouse inertia** — cube coasts after a flick and gradually decelerates
- **Scroll zoom** — camera distance 0.5–20 units
- **Fullscreen** — borderless native resolution via `Alt+Enter` or settings checkbox, restores window on toggle back
- **Auto-load texture** — drops `texture.png/.jpg/.bmp/.tga` next to the `.exe`; falls back to a built-in checkerboard
- **Standalone exe** — all libraries statically linked, no DLLs required
- **FPS counter** — color-coded overlay (green / yellow / red)
- **Wireframe mode** — clean 12-edge line drawing (no triangle diagonals)

## Requirements

- Windows 10/11 (x64)
- [Visual Studio 2019 or 2022](https://visualstudio.microsoft.com/) with **Desktop development with C++** workload  
  *(or just the Build Tools — `winget install Microsoft.VisualStudio.2022.BuildTools`)*
- [CMake ≥ 3.15](https://cmake.org/) — `winget install Kitware.CMake`
- [vcpkg](https://github.com/microsoft/vcpkg)

## First-time Setup

### 1. Install vcpkg (one-time)

```powershell
git clone https://github.com/microsoft/vcpkg C:\vcpkg
C:\vcpkg\bootstrap-vcpkg.bat
```

### 2. Configure the project

vcpkg will automatically download and compile all dependencies (GLFW, GLEW, GLM, ImGui):

```powershell
cd path\to\cgame
cmake -B build -S . `
  -DCMAKE_TOOLCHAIN_FILE=C:\vcpkg\scripts\buildsystems\vcpkg.cmake `
  -DVCPKG_TARGET_TRIPLET=x64-windows-static
```

> First run takes a few minutes while vcpkg builds the libraries.

### 3. Build

**Release** (standalone `.exe`, no DLLs required):
```powershell
cmake --build build --config Release
# Output: bin\Release\cgame.exe
```

**Debug:**
```powershell
cmake --build build --config Debug
# Output: bin\Debug\cgame.exe
```

## Development Workflow

After editing `src/main.cpp`, just rebuild — no need to reconfigure:

```powershell
cmake --build build --config Release ; .\bin\Release\cgame.exe
```

Only re-run the `cmake -B build ...` configure step when:
- Adding new dependencies to `vcpkg.json`
- Changing `CMakeLists.txt`

## Dependencies

| Library | Version | Purpose |
|---|---|---|
| [GLFW](https://www.glfw.org/) | 3.4 | Window, input & fullscreen |
| [GLEW](https://glew.sourceforge.net/) | 2.3.1 | OpenGL extension loader |
| [GLM](https://github.com/g-truc/glm) | 1.0.3 | Math (matrices, vectors) |
| [Dear ImGui](https://github.com/ocornut/imgui) | 1.92 | Settings UI |
| [stb_image](https://github.com/nothings/stb) | 2024-07 | PNG/JPG/BMP/TGA texture loading |

All dependencies are statically linked — the final `.exe` is fully self-contained.
