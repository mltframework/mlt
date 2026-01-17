# Use Ubuntu 24.04 (Noble Numbat) as the base for the full toolchain
FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive
ENV LANG=C.UTF-8
ENV LC_ALL=C.UTF-8

RUN apt-get update -qq && apt-get install -yqq \
    # 1. System Tools, Compilation, and High-Performance Linker (mold)
    apt-utils git wget sudo curl build-essential cmake ninja-build \
    pkg-config gdb clang-format-14 swig meson yasm nasm \
    valgrind kcachegrind bash-completion cppcheck \
    automake autoconf libtool intltool xutils-dev mold \
    python3-dev python3-pip python3-setuptools kwalify \
    && apt-get clean && rm -rf /var/lib/apt/lists/*

# Force the system to use mold as the default linker for performance
RUN ln -sf /usr/bin/mold /usr/bin/ld
# Required for MLT format-style-code scripts
RUN ln -s /usr/bin/clang-format-14 /usr/bin/clang-format

RUN apt-get update -qq && apt-get install -yqq \
    # 2. Multimedia Core (AV1, HDR, SRT, RIST, OCIO)
    libavformat-dev libavcodec-dev libavdevice-dev libavfilter-dev \
    libswscale-dev libswresample-dev libpostproc-dev \
    libaom-dev libdav1d-dev libsvtav1-dev libvpx-dev \
    libx264-dev libx265-dev libsrt-openssl-dev librist-dev \
    libsamplerate0-dev libxml2-dev libarchive-dev libexif-dev \
    libfftw3-dev libeigen3-dev libebur128-dev libbs2b-dev \
    libopencolorio-dev libwebp-dev \
    # 3. GPU & Visual (Movit, Vulkan, OpenCV, Vid.Stab, Frei0r)
    libgl1-mesa-dev libegl1-mesa-dev libvulkan-dev vulkan-tools \
    libmovit-dev libopencv-dev libgavl-dev libvidstab-dev \
    libgdk-pixbuf-2.0-dev frei0r-plugins-dev \
    # 4. Modern Qt6 Stack (Essential for MLT 7+ and Glaxnimate support)
    qt6-base-dev qt6-5compat-dev qt6-svg-dev qt6-multimedia-dev \
    qt6-shadertools-dev qt6-declarative-dev qt6-l10n-tools \
    qt6-tools-dev libqt6core6t64 libqt6gui6t64 libqt6widgets6t64 \
    libqt6opengl6-dev libqt6network6t64 \
    # 5. Audio Stack (PipeWire, Pulse, Jack, Rubberband, LV2 support)
    libpipewire-0.3-dev libspa-0.2-dev libjack-jackd2-dev libpulse-dev \
    libasound2-dev liblilv-dev libserd-dev libsord-dev libsratom-dev \
    libsox-dev libtheora-dev libvorbis-dev libopus-dev \
    libflac-dev librubberband-dev ladspa-sdk swh-plugins \
    tap-plugins cmt blepvco liblo-dev librtaudio-dev \
    libspatialaudio-dev libsdl1.2-dev libsdl2-dev \
    # 6. Vector Graphics, Fonts and Headless Runtime
    libjsoncpp-dev libjson-c-dev xvfb libxkbcommon-x11-0 \
    fonts-liberation fonts-noto-core libfontconfig1-dev \
    libfribidi-dev libharfbuzz-dev libpango1.0-dev \
    # 7. Documentation & Graph Tools (For BUILD_DOCS)
    doxygen graphviz \
    # 8. Extra Dependencies for Full Option Coverage (Bindings & Glaxnimate)
    libpotrace-dev mono-devel mono-mcs locales openjdk-17-jdk libnode-dev \
    tcl-dev tk-dev ruby-dev liblua5.3-dev php-dev perl libperl-dev \
    # 9. Hardware Acceleration Runtimes (VA-API / VDPAU)
    libva-dev libvdpau-dev libva-drm2 libva-x11-2 \
    && apt-get clean && rm -rf /var/lib/apt/lists/*

# Generate required locales for MLT unit tests (Mandatory for RadixRespondsToLocale)
RUN locale-gen en_US.UTF-8 && \
    locale-gen de_DE.UTF-8 && \
    update-locale

# FIX FOR MONO: Ensure mcs is visible if the package doesn't link it correctly
RUN ln -sf /usr/bin/mcs /usr/local/bin/mcs

# 10. User and group setup for DevContainer compatibility
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

# Environment setup
ENV PATH="/usr/lib/qt6/bin:${PATH}"
ENV MLT_REPOSITORY=/usr/local/lib/mlt-7
ENV LD_LIBRARY_PATH=/usr/local/lib:$LD_LIBRARY_PATH
# Force mold linker for CMake builds
ENV LDFLAGS="-fuse-ld=mold"
# Headless audio driver for CI/Tests
ENV SDL_AUDIODRIVER=dummy

# FIX FOR JNI: Explicitly set JAVA_HOME for CMake FindJNI
ENV JAVA_HOME=/usr/lib/jvm/java-17-openjdk-amd64
ENV CPATH="$JAVA_HOME/include:$JAVA_HOME/include/linux:$CPATH"

USER USERNAME
WORKDIR /workspaces