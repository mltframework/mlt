FROM ubuntu:20.04 AS base

ENV DEBIAN_FRONTEND noninteractive
ENV HOME /tmp

RUN apt-get update -qq && apt-get install -yqq apt-utils

FROM base AS build

# Install packages for building
RUN apt-get install -yqq wget git automake autoconf libtool intltool g++ yasm nasm \
  swig libgavl-dev libsamplerate0-dev libxml2-dev ladspa-sdk libjack-dev \
  libsox-dev libsdl2-dev libgtk2.0-dev libsoup2.4-dev \
  qt5-default libqt5webkit5-dev libqt5svg5-dev \
  libexif-dev libtheora-dev libvorbis-dev python3-dev cmake xutils-dev \
  libegl1-mesa-dev libeigen3-dev libfftw3-dev libvdpau-dev meson ninja-build

# Get and run the build script
RUN wget --quiet -O /tmp/build-melt.sh https://raw.githubusercontent.com/mltframework/mlt-scripts/master/build/build-melt.sh && \
  echo "INSTALL_DIR=\"/usr/local\"" > /tmp/build-melt.conf && \
  echo "SOURCE_DIR=\"/tmp/melt\"" >> /tmp/build-melt.conf && \
  echo "AUTO_APPEND_DATE=0" >> /tmp/build-melt.conf && \
  echo "FFMPEG_HEAD=0" >> /tmp/build-melt.conf && \
  echo "FFMPEG_REVISION=origin/release/5.0" >> /tmp/build-melt.conf && \
  bash /tmp/build-melt.sh -c /tmp/build-melt.conf


FROM base

# Install packages for running
RUN apt-get install -yqq dumb-init \
  libsamplerate0 libxml2 libjack0 \
  libsdl2-2.0-0 libgtk2.0-0 libsoup2.4-1 \
  libqt5core5a libqt5gui5 libqt5opengl5 libqt5svg5 libqt5widgets5 \
  libqt5x11extras5 libqt5xml5 libqt5webkit5 \
  libtheora0 libvorbis0a python3 \
  libegl1-mesa libfftw3-3 libvdpau1 \
  # Additional runtime libs \
  libgavl1 libsox3 libexif12 xvfb libxkbcommon-x11-0 libhyphen0 libwebp6 \
  # LADSPA plugins \
  amb-plugins ambdec autotalent blepvco blop bs2b-ladspa caps cmt \
  csladspa fil-plugins guitarix-ladspa invada-studio-plugins-ladspa mcp-plugins \
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
