# cgame — 3D OpenGL Viewer

A GPU-accelerated 3D OpenGL app written in C++ that renders a textured, lit cube **or any loaded 3D model** with a full ImGui settings panel.

## Controls

| Input | Action |
|---|---|
| `↑ ↓ ← →` | Rotate object |
| `LMB drag` | Free rotate (when auto-rotate is off) |
| `MMB drag` | Pan camera |
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
| **3D Model** | Browse & load OBJ/FBX/3DS/GLTF/DAE/PLY/STL, Show model toggle, Scale slider |
| **Texture** | Browse any PNG/JPG/BMP/TGA (for cube), Use as bump map, Bump strength slider |
| **Camera** | Distance slider + scroll wheel zoom |

## Light Controls Overlay (bottom-right, always visible)

| Control | Description |
|---|---|
| Light on/off | Enable / disable dynamic lighting |
| Wire | Toggle wireframe |
| Dir XYZ | Light direction vector |
| Color | Light color picker |
| Ambient | Ambient light strength |
| Diffuse | Diffuse light strength |
| Spec / Shin | Specular strength and shininess |
| Reset light | Restore default values |

## Features

- **3D model loading** — OBJ, FBX, 3DS, GLTF/GLB, DAE, PLY, STL via Assimp; auto-centers and auto-scales on load
- **Per-mesh textures** — automatically loads diffuse, normal map, and gloss maps from model material data; searches `tex/` subfolder automatically
- **Alpha mask support** — per-fragment discard for transparent cutouts (hair, eyelashes)
- **Middle-mouse pan** — pan the camera in screen space, speed scales with zoom level
- **Blinn-Phong lighting** — separate ambient/diffuse/specular controls; fully adjustable from the always-visible overlay
- **Normal map support** — real TBN-based normal mapping from loaded `_norm` textures
- **Gloss map support** — per-texel specular intensity from loaded `_gloss` textures
- **Bump mapping** — height-field bump from texture luminance (for cube mode)
- **Mouse inertia** — object coasts after a flick and gradually decelerates
- **Scroll zoom** — camera distance 0.5–20 units
- **Fullscreen** — borderless native resolution via `Alt+Enter` or settings checkbox, restores window on toggle back
- **Auto-load texture** — drops `texture.png/.jpg/.bmp/.tga` next to the `.exe`; falls back to a built-in checkerboard
- **Standalone exe** — all libraries statically linked, no DLLs required
- **No imgui.ini** — settings panel does not create config files on disk
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

vcpkg will automatically download and compile all dependencies:

```powershell
cd path\to\cgame
cmake -B build -S . `
  -DCMAKE_TOOLCHAIN_FILE=C:\vcpkg\scripts\buildsystems\vcpkg.cmake `
  -DVCPKG_TARGET_TRIPLET=x64-windows-static
```

> First run takes several minutes while vcpkg builds the libraries (including Assimp).

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
| [Assimp](https://github.com/assimp/assimp) | 5.x | 3D model loading (40+ formats) |

All dependencies are statically linked — the final `.exe` is fully self-contained.
