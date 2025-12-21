# PCSX ReARMed ARM Build Environment using crosstool-ng
# Builds pcsx_rearmed emulator for PlayStation Classic with custom toolchain
#
# Build: docker build -t pcsx-rearmed .
# Extract:
#   id=$(docker create pcsx-rearmed)
#   docker cp $id:/build/output/. ./pcsx_bin/
#   docker rm $id
#
# Output directory contains:
#   pcsx-ab              - Main emulator binary (PCSX AutoBleem)
#   plugins/gpu_peops.so - P.E.Op.S GPU plugin
#   plugins/gpu_unai.so  - UNAI GPU plugin
#
# Uses libpicofe-psc submodule: https://github.com/AutoBleem-NG/libpicofe-psc (branch: psc-autobleem)
# Audio is built-in via dfsound SPU with SDL backend (no separate SPU plugin needed)
#

# ==============================================================================
# Version Configuration
# ==============================================================================
ARG CROSSTOOL_NG_VERSION=1.28.0

# Toolchain versions - matched for PlayStation Classic compatibility
ARG CT_LINUX_VERSION=4_4
ARG CT_BINUTILS_VERSION=2_32
ARG CT_GLIBC_VERSION=2_23
ARG CT_GCC_VERSION=9

# UPX version for binary compression
ARG UPX_VERSION=5.0.2

# User IDs for crosstool-ng build
ARG CTNG_UID=1000
ARG CTNG_GID=1000

# ==============================================================================
# Stage 1: Build Custom GCC Toolchain with crosstool-ng
# ==============================================================================
FROM ubuntu:16.04 AS ctngbuild

ARG CTNG_UID
ARG CTNG_GID
ARG CROSSTOOL_NG_VERSION
ARG CT_LINUX_VERSION
ARG CT_BINUTILS_VERSION
ARG CT_GLIBC_VERSION
ARG CT_GCC_VERSION

# Create user for crosstool-ng (cannot run as root)
RUN groupadd -g $CTNG_GID ctng && \
    useradd -d /home/ctng -m -g $CTNG_GID -u $CTNG_UID -s /bin/bash ctng

# Install crosstool-ng build dependencies
RUN apt-get update && \
    apt-get install -y \
        gcc g++ gperf bison flex texinfo help2man make libncurses5-dev \
        python3-dev autoconf automake libtool libtool-bin gawk wget bzip2 \
        xz-utils unzip patch libstdc++6 rsync meson ninja-build && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/*

# Setup crosstool-ng directories
RUN mkdir /opt/ctng && chmod 777 /opt/ctng && \
    mkdir /opt/x-tools && chmod 777 /opt/x-tools && \
    echo 'export PATH=/opt/ctng/bin:$PATH' >> /etc/profile

USER ctng

# Download and build crosstool-ng
RUN wget -O /tmp/crosstool.bz2 http://crosstool-ng.org/download/crosstool-ng/crosstool-ng-${CROSSTOOL_NG_VERSION}.tar.bz2 && \
    cd /home/ctng && tar xvf /tmp/crosstool.bz2 && \
    rm /tmp/crosstool.bz2 && \
    cd /home/ctng/crosstool-ng-${CROSSTOOL_NG_VERSION} && \
    ./configure --prefix=/opt/ctng && \
    make && \
    make install

# Configure and build ARM toolchain for PlayStation Classic
# - ARMv7VE with NEON (compatible with ARMv8-A in AArch32 mode)
# - Hard float ABI
RUN echo 'CT_CONFIG_VERSION="4"' >> /tmp/defconfig && \
    echo 'CT_PREFIX_DIR="/opt/x-tools/${CT_HOST:+HOST-${CT_HOST}/}${CT_TARGET}"' >> /tmp/defconfig && \
    echo 'CT_ARCH_ARM=y' >> /tmp/defconfig && \
    echo 'CT_OMIT_TARGET_VENDOR=y' >> /tmp/defconfig && \
    echo 'CT_ARCH_FLOAT_HW=y' >> /tmp/defconfig && \
    echo 'CT_KERNEL_LINUX=y' >> /tmp/defconfig && \
    echo 'CT_LINUX_V_'${CT_LINUX_VERSION}'=y' >> /tmp/defconfig && \
    echo 'CT_BINUTILS_V_'${CT_BINUTILS_VERSION}'=y' >> /tmp/defconfig && \
    echo 'CT_GLIBC_V_'${CT_GLIBC_VERSION}'=y' >> /tmp/defconfig && \
    echo 'CT_GCC_V_'${CT_GCC_VERSION}'=y' >> /tmp/defconfig && \
    echo 'CT_CC_LANG_CXX=y' >> /tmp/defconfig && \
    echo 'CT_CC_GCC_LIBGOMP=y' >> /tmp/defconfig && \
    cd /tmp && /opt/ctng/bin/ct-ng defconfig && \
    echo 'CT_ZLIB_MIRRORS="http://downloads.sourceforge.net/project/libpng/zlib/${CT_ZLIB_VERSION} https://www.zlib.net/ https://www.zlib.net/fossils"' >> /tmp/.config && \
    cd /tmp && /opt/ctng/bin/ct-ng build

# ==============================================================================
# Stage 2: Build PCSX ReARMed for ARM
# ==============================================================================
FROM ubuntu:18.04

LABEL maintainer="AutoBleem Team"
LABEL description="Docker build environment for PCSX ReARMed - PlayStation Classic (crosstool-ng)"

ENV DEBIAN_FRONTEND="noninteractive"

# Install base build dependencies
ENV PACKAGES=" \
    gcc-arm-linux-gnueabihf \
    g++-arm-linux-gnueabihf \
    pkgconf \
    pkg-config-arm-linux-gnueabihf \
    crossbuild-essential-armhf \
    git \
    build-essential \
    make \
    wget \
    curl \
    xz-utils"

RUN apt-get update && \
    apt-get install -y $PACKAGES && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/*

# Download and install UPX for binary compression
ARG UPX_VERSION
RUN wget -q https://github.com/upx/upx/releases/download/v${UPX_VERSION}/upx-${UPX_VERSION}-amd64_linux.tar.xz && \
    tar -xf upx-${UPX_VERSION}-amd64_linux.tar.xz && \
    cp upx-${UPX_VERSION}-amd64_linux/upx /usr/local/bin/ && \
    chmod +x /usr/local/bin/upx && \
    rm -rf upx-${UPX_VERSION}-amd64_linux upx-${UPX_VERSION}-amd64_linux.tar.xz

# Install ARM libraries needed for PCSX ReARMed (including EGL/GLES)
ENV ARM_PACKAGES=" \
    libasound2-dev:armhf \
    libsdl2-dev:armhf \
    zlib1g-dev:armhf \
    libpng-dev:armhf \
    libwayland-dev:armhf \
    libdrm-dev:armhf \
    libegl1-mesa-dev:armhf \
    libgles2-mesa-dev:armhf \
    libglvnd-dev"

RUN dpkg --add-architecture armhf && \
    mv /etc/apt/sources.list /etc/apt/sources.list.bak && \
    echo "deb [arch=amd64] http://archive.ubuntu.com/ubuntu bionic main universe" > /etc/apt/sources.list && \
    echo "deb [arch=amd64] http://archive.ubuntu.com/ubuntu bionic-updates main universe" >> /etc/apt/sources.list && \
    echo "deb [arch=armhf] http://ports.ubuntu.com/ubuntu-ports bionic main universe" >> /etc/apt/sources.list && \
    echo "deb [arch=armhf] http://ports.ubuntu.com/ubuntu-ports bionic-updates main universe" >> /etc/apt/sources.list && \
    apt-get update && \
    apt-get install -y $ARM_PACKAGES && \
    apt-get -y autoremove && \
    apt-get -y clean && \
    mkdir -p /opt/x-tools && \
    rm -rf /var/lib/apt/lists/*

# Download GLES1 headers from Khronos (not available in Ubuntu 18.04 armhf)
RUN mkdir -p /usr/include/GLES && \
    curl -sL --retry 3 https://registry.khronos.org/OpenGL/api/GLES/gl.h -o /usr/include/GLES/gl.h && \
    curl -sL --retry 3 https://registry.khronos.org/OpenGL/api/GLES/glext.h -o /usr/include/GLES/glext.h && \
    curl -sL --retry 3 https://registry.khronos.org/OpenGL/api/GLES/glplatform.h -o /usr/include/GLES/glplatform.h

# Copy the custom toolchain from stage 1
COPY --from=ctngbuild /opt/x-tools/arm-linux-gnueabihf /opt/x-tools/arm-linux-gnueabihf

# Set up environment for cross-compilation
ENV PATH="/opt/x-tools/arm-linux-gnueabihf/bin:${PATH}"
ENV PKG_CONFIG_PATH=/usr/lib/arm-linux-gnueabihf/pkgconfig

# Create wrapper scripts for the armv8-sony prefix that include the Ubuntu ARM sysroot paths
# The crosstool-ng compiler needs to find Ubuntu's ARM libraries
RUN echo '#!/bin/bash' > /usr/bin/armv8-sony-linux-gnueabihf-gcc && \
    echo 'exec /opt/x-tools/arm-linux-gnueabihf/bin/arm-linux-gnueabihf-gcc -idirafter /usr/include -idirafter /usr/include/arm-linux-gnueabihf -L/usr/lib/arm-linux-gnueabihf "$@"' >> /usr/bin/armv8-sony-linux-gnueabihf-gcc && \
    chmod +x /usr/bin/armv8-sony-linux-gnueabihf-gcc && \
    echo '#!/bin/bash' > /usr/bin/armv8-sony-linux-gnueabihf-g++ && \
    echo 'exec /opt/x-tools/arm-linux-gnueabihf/bin/arm-linux-gnueabihf-g++ -idirafter /usr/include -idirafter /usr/include/arm-linux-gnueabihf -L/usr/lib/arm-linux-gnueabihf "$@"' >> /usr/bin/armv8-sony-linux-gnueabihf-g++ && \
    chmod +x /usr/bin/armv8-sony-linux-gnueabihf-g++ && \
    ln -sf /opt/x-tools/arm-linux-gnueabihf/bin/arm-linux-gnueabihf-ar /usr/bin/armv8-sony-linux-gnueabihf-ar && \
    ln -sf /opt/x-tools/arm-linux-gnueabihf/bin/arm-linux-gnueabihf-as /usr/bin/armv8-sony-linux-gnueabihf-as

# Create sdl2-config wrapper that returns ARM SDL2 flags
# Use --allow-shlib-undefined since SDL2's X11/Wayland/PulseAudio deps resolve at runtime
RUN printf '#!/bin/sh\ncase "$1" in\n  --cflags) echo "-I/usr/include/SDL2 -D_REENTRANT" ;;\n  --libs) echo "-L/usr/lib/arm-linux-gnueabihf -Wl,--allow-shlib-undefined -lSDL2 -lpthread" ;;\n  *) echo "-I/usr/include/SDL2 -D_REENTRANT -L/usr/lib/arm-linux-gnueabihf -Wl,--allow-shlib-undefined -lSDL2 -lpthread" ;;\nesac\n' > /usr/bin/sdl2-config && \
    chmod +x /usr/bin/sdl2-config && \
    mkdir -p /opt/x-tools/arm-linux-gnueabihf/arm-linux-gnueabihf/sysroot/usr/bin && \
    cp /usr/bin/sdl2-config /opt/x-tools/arm-linux-gnueabihf/arm-linux-gnueabihf/sysroot/usr/bin/sdl2-config

# Set SDL_CONFIG to use our wrapper directly
ENV SDL_CONFIG=/usr/bin/sdl2-config

# Copy source code into container (supports both repo root and pcsx_rearmed/ subdirectory)
WORKDIR /build/pcsx_rearmed
COPY . .

# Build PCSX ReARMed for ARM using Makefile.psc
WORKDIR /build/pcsx_rearmed
# Clean all object files (in case of stale cached builds with wrong arch)
RUN find . -name '*.o' -delete && find . -name '*.a' -delete && \
    rm -f config.mak config.log pcsx pcsx-ab include/revision.h plugins/*/*.so

# Patch Makefile to use gl_platform_psc.o (PSC Wayland/EGL backend from libpicofe-psc)
RUN sed -i 's|frontend/libpicofe/gl_platform\.o|frontend/libpicofe/gl_platform_psc.o|g' Makefile

# Create revision.h with build version (passed from Makefile via --build-arg)
ARG GIT_VERSION="AutoBleem-NG"
RUN mkdir -p include && \
    echo "#define REV \"${GIT_VERSION}\"" > include/revision.h

RUN SDL_CONFIG=/usr/bin/sdl2-config DUMP_CONFIG_LOG=1 make -f Makefile.psc arm JOBS=$(nproc)

# Compress binary with UPX for smaller size and faster loading
# Note: Binary is already stripped by -s linker flag in config.mak.psc
RUN echo "Before UPX:" && ls -la pcsx-ab && \
    upx -9 pcsx-ab && \
    echo "After UPX:" && ls -la pcsx-ab

# Collect all outputs to a single directory for easy extraction
# Note: Audio is built-in (dfsound SPU with SDL backend) - no separate SPU plugin needed
RUN mkdir -p /build/output/plugins && \
    cp pcsx-ab /build/output/ && \
    cp plugins/dfxvideo/gpu_peops.so /build/output/plugins/ 2>/dev/null || true && \
    cp plugins/gpu_unai/gpu_unai.so /build/output/plugins/ 2>/dev/null || true && \
    ls -laR /build/output/

CMD ["/bin/bash"]
