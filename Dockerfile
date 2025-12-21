FROM ubuntu:24.04 AS base

ENV DEBIAN_FRONTEND noninteractive
ENV HOME /tmp

RUN apt-get update -qq && apt-get install -yqq apt-utils

FROM base AS build

# Install packages for building
RUN apt-get install -yqq wget git automake autoconf libtool intltool g++ yasm nasm \
  swig libgavl-dev libsamplerate0-dev libxml2-dev ladspa-sdk libjack-dev \
  libsox-dev libsdl2-dev libgtk2.0-dev libsoup2.4-dev \
  qt6-base-dev qt6-svg-dev libarchive-dev libmp3lame-dev \
  libexif-dev libtheora-dev libvorbis-dev python3-dev cmake xutils-dev \
  libegl1-mesa-dev libeigen3-dev libfftw3-dev libvdpau-dev meson ninja-build

# Get and run the build script
RUN wget --quiet -O /tmp/build-melt.sh https://raw.githubusercontent.com/mltframework/mlt-scripts/master/build/build-melt.sh && \
  echo "INSTALL_DIR=\"/usr/local\"" > /tmp/build-melt.conf && \
  echo "SOURCE_DIR=\"/tmp/melt\"" >> /tmp/build-melt.conf && \
  echo "AUTO_APPEND_DATE=0" >> /tmp/build-melt.conf && \
  bash /tmp/build-melt.sh -c /tmp/build-melt.conf


FROM base

# Install packages for running
RUN apt-get install -yqq dumb-init \
  libsamplerate0 libxml2 libjack0 \
  libsdl2-2.0-0 libgtk2.0-0 libsoup2.4-1 \
  libqt6svgwidgets6 libqt6xml6 qt6-image-formats-plugins libarchive13 \
  libtheora0 libvorbis0a python3 \
  libegl1 libfftw3-double3 libvdpau1 \
  # Additional runtime libs \
  libgavl2 libsox3 libexif12 xvfb libxkbcommon-x11-0 libhyphen0 libwebp7 \
  # LADSPA plugins \
  amb-plugins ambdec autotalent blepvco blop bs2b-ladspa caps cmt \
  csladspa fil-plugins invada-studio-plugins-ladspa mcp-plugins \
  omins rev-plugins ste-plugins swh-plugins tap-plugins vco-plugins wah-plugins \
  lsp-plugins-ladspa dpf-plugins-ladspa \
  # Fonts \
  fonts-liberation 'ttf-.+'

# Install the build
COPY --from=build /usr/local/ /usr/local/

WORKDIR /mnt
ENV LD_LIBRARY_PATH /usr/local/lib

# Qt, Movit, and WebVfx require xvfb-run, which requires a PID 1 init provided by dumb-init
ENTRYPOINT ["/usr/bin/dumb-init", "--", "/usr/bin/xvfb-run", "-a", "/usr/local/bin/melt"]
