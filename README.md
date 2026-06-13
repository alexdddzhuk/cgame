# cgame — 3D OpenGL Cube

A simple 3D OpenGL app written in C++ that renders a rotating colored cube with an ImGui settings panel.

## Controls

| Input | Action |
|---|---|
| `↑ ↓ ← →` | Rotate cube |
| `ESC` | Quit |
| `=` button (top-left) | Open / close settings panel |

## Settings Panel

- **Rotation** — manual pitch/yaw sliders + reset
- **Auto-Rotate** — enable with configurable X/Y speed
- **Keyboard** — arrow key rotation speed
- **Display** — wireframe toggle, cube scale, background color
- **Info** — live FPS counter

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
| [GLFW](https://www.glfw.org/) | 3.4 | Window & input |
| [GLEW](https://glew.sourceforge.net/) | 2.3.1 | OpenGL extension loader |
| [GLM](https://github.com/g-truc/glm) | 1.0.3 | Math (matrices, vectors) |
| [Dear ImGui](https://github.com/ocornut/imgui) | 1.92 | Settings UI |

All dependencies are statically linked — the final `.exe` is fully self-contained.
