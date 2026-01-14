# MLT FRAMEWORK

MLT is a LGPL multimedia framework designed for video editing.

This document provides a quick reference for the minimal configuration, build and
installation of MLT.

See the `docs/` directory for usage details.

See the [website](https://www.mltframework.org/docs/) for development details
and a [contributing](https://www.mltframework.org/docs/contributing/) guide.

---

## Development Environment (Dev Container)

This repository provides a standardized development environment using the [Development Containers](https://containers.dev/) specification. It is compatible with **VS Code**, **CLion**, **DevPod**, and other IDEs supporting the `.devcontainer` standard.

### Prerequisites
- Docker (or Podman)
- An IDE with Dev Container support.

### Hardware Acceleration and Audio Support
By default, the container runs in **Restricted Mode** (secure/headless). In this mode:
* Audio is routed to a `dummy` driver (no sound).
* GPU access might be limited to software rendering.

#### Enabling Real Hardware Access
To enable **Full GPU Acceleration (DRI)** and **Physical Audio (ALSA/PulseAudio/PipeWire)**, follow these steps:

1.  **Modify `devcontainer.json`**: Uncomment the following lines inside the `runArgs` array:
    ```json
    "--ipc=host",
    "--privileged"
    ```
    * **What this does**: `--ipc=host` allows shared memory access for high-performance video frames, and `--privileged` grants the container direct access to host devices (GPU nodes and Sound Cards).

2.  **Trigger a Rebuild**: Changes to `runArgs` require a container recreation. 
    * In VS Code: Run the command `Dev Containers: Rebuild Container`.
    * In CLion: Select `Restart and Rebuild Container`.

3.  **Switch the Audio Driver**: The `Dockerfile` defaults to `SDL_AUDIODRIVER=dummy`. Once in privileged mode, you must tell the MLT consumer to use a real driver:
    * **Temporary (CLI)**:
    ```bash
    SDL_AUDIODRIVER=alsa melt video.mp4
    ```
    * **Permanent**: Edit the `ENV SDL_AUDIODRIVER` line in your `.devcontainer/Dockerfile` to `alsa` or `pulseaudio`.

> **Warning**: Using `--privileged` mode reduces the security isolation between the container and your host machine. Use it only when real-time hardware playback is required for development.

---
## Configuration

Configuration is triggered by running CMake **from inside a build directory**, not from the project root.

```bash
mkdir -p build
cd build
cmake ..
```

Alternatively, you can configure CMake with Ninja:

```bash
cmake -G Ninja ..
```

More information on usage is found by viewing `CMakeLists.txt` and the CMake manual page.

---

## Compilation

Once configured, it should be sufficient to run:

```bash
cmake --build .
```

Alternatively, you can compile with Ninja:

```bash
ninja
```

Alternatively, you can compile using CMake or Ninja with one less CPU core (recommended only on systems with many CPU cores):


```bash
cmake --build . --parallel $(($(nproc) - 1))
# or
ninja -j $(($(nproc) - 1))
```

All generated files (Makefiles, Ninja files, cache, object files, binaries) will remain inside the `build/` directory.

---

## Testing

To execute the MLT tools without installation, or to test a new version on a system with an already installed MLT version, run:

```bash
source ../setenv
```

NB:

* This applies to the **current shell only**
* It assumes a **bash or Bourne-compatible shell** is in use
* The command must be executed from inside the `build/` directory

---

## Installation

Installation is triggered by running:

```bash
sudo cmake --install .
```

This installs the files generated in the `build/` directory, without affecting the project source tree.

> **Note**: After installation, some Linux systems may require running `sudo ldconfig`
> to refresh the shared library cache, especially if the install prefix is not already
> part of the system linker configuration.


---

## Summary

```bash
mkdir build
cd build
cmake ..
cmake --build .
source ../setenv
sudo cmake --install .
```

This approach keeps the source tree clean and allows the entire build to be removed safely by deleting the `build/` directory.

---

# More Information

For more detailed information, please refer to https://mltframework.org/docs/install/.