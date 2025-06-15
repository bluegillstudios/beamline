# Beamline

**Beamline** is a fast, modular, command-line raytracer designed for both CPU-based path tracing and hybrid GPU acceleration. Built for researchers, graphics enthusiasts, and hackers who want full control — from the shell.

![Render Example](docs/cornell.png) 

---

## Features

- Physically-based path tracing (CPU)
- GPU hybrid support (OpenCL/CUDA backend, optional)
- Modular scene definition and material system
- Multithreaded rendering engine
- Simple CLI with progress feedback
- Output to PNG or PPM
- Scene loading from `.beam` or `.json` (WIP)

---

## Build Instructions

### Requirements
- C++17 or later
- CMake 3.16+
- OpenMP (for CPU multithreading)
- Optional: OpenCL or CUDA for GPU backend
- stb_image and stb_image_write (included as submodules)

### Build

```bash
git clone https://github.com/bluegillstudios/beamline
cd beamline
mkdir build && cd build
cmake ..
make -j$(nproc)
```

## Usage 
```bash
./beamline render --scene <scene> --output <filename> --spp 256 --device <device>
```
