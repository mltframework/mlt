FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive
ENV LANG=C.UTF-8
ENV LC_ALL=C.UTF-8

# 1. System Tools, Compilation, and High-Performance Linker (mold)
RUN apt-get update -qq && apt-get install -yqq \
    apt-utils git wget sudo curl build-essential cmake ninja-build \
    pkg-config gdb clang-format swig meson yasm nasm \
    automake autoconf libtool intltool xutils-dev mold \
    python3-dev python3-pip python3-setuptools \
    # 2. Multimedia Core (AV1, HDR, SRT, RIST)
    libavformat-dev libavcodec-dev libavdevice-dev libavfilter-dev \
    libswscale-dev libswresample-dev libpostproc-dev \
    libaom-dev libdav1d-dev libsvtav1-dev libvpx-dev \
    libx264-dev libx265-dev libsrt-openssl-dev librist-dev \
    libsamplerate0-dev libxml2-dev libarchive-dev libexif-dev \
    libfftw3-dev libeigen3-dev libebur128-dev libbs2b-dev \
    # 3. GPU & Visual (Movit, Vulkan, OpenCV, Vid.Stab, Frei0r)
    libgl1-mesa-dev libegl1-mesa-dev libvulkan-dev vulkan-tools \
    libmovit-dev libopencv-dev libgavl-dev libvidstab-dev \
    libgdk-pixbuf-2.0-dev frei0r-plugins-dev \
    # 4. Modern Qt6 Stack (Essential for MLT 7+)
    qt6-base-dev qt6-5compat-dev qt6-svg-dev qt6-multimedia-dev \
    qt6-shadertools-dev qt6-declarative-dev qt6-l10n-tools \
    libqt6core6 libqt6gui6 libqt6widgets6 libqt6opengl6-dev \
    # 5. Audio Stack (PipeWire, SPA, Jack, Rubberband)
    libpipewire-0.3-dev libspa-0.2-dev libjack-jackd2-dev \
    libsox-dev libsdl2-dev libtheora-dev libvorbis-dev libopus-dev \
    libflac-dev librubberband-dev ladspa-sdk swh-plugins \
    tap-plugins cmt blepvco liblo-dev \
    # 6. Vector Graphics and Headless Runtime
    libarchive-dev libjsoncpp-dev xvfb libxkbcommon-x11-0 \
    fonts-liberation fonts-noto-core libfontconfig1-dev \
    libfribidi-dev libharfbuzz-dev \
    # 7. Cleanup
    && apt-get clean && rm -rf /var/lib/apt/lists/*

# User Configuration and Hardware Groups
ARG USERNAME=vscode
ARG USER_UID=1000
ARG USER_GID=$USER_UID

RUN if getent group $USER_GID; then \
        groupmod -n $USERNAME $(getent group $USER_GID | cut -d: -f1); \
    else \
        groupadd --gid $USER_GID $USERNAME; \
    fi \
    && if getent passwd $USER_UID; then \
        usermod -l $USERNAME -m -d /home/$USERNAME $(getent passwd $USER_UID | cut -d: -f1); \
    else \
        useradd --uid $USER_UID --gid $USER_GID -m $USERNAME; \
    fi \
    && usermod -aG sudo $USERNAME \
    && groupadd -f video && usermod -aG video $USERNAME \
    && groupadd -f render && usermod -aG render $USERNAME \
    && echo $USERNAME ALL=\(root\) NOPASSWD:ALL > /etc/sudoers.d/$USERNAME \
    && chmod 0440 /etc/sudoers.d/$USERNAME

# Environment Variables
ENV PATH="/usr/lib/qt6/bin:${PATH}"
ENV MLT_REPOSITORY=/usr/local/lib/mlt-7
ENV LD_LIBRARY_PATH=/usr/local/lib:$LD_LIBRARY_PATH
# dummy driver avoids errors in headless environments or CI
ENV SDL_AUDIODRIVER=dummy

USER $USERNAME
WORKDIR /workspaces