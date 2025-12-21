# PlayStation Classic build configuration
# Pre-generated config.mak for PSC ARM build (Wayland/EGL/GLES)
# This bypasses configure since we need specific settings for PSC

# Output binary name (pcsx-ab = PCSX AutoBleem)
TARGET = pcsx-ab

CC = armv8-sony-linux-gnueabihf-gcc
CXX = armv8-sony-linux-gnueabihf-g++
AS = armv8-sony-linux-gnueabihf-as
AR = armv8-sony-linux-gnueabihf-ar

# CFLAGS for PSC build
# - SDL_VIDEO_DRIVER_WAYLAND: Force SDL2 to use Wayland backend
# - HAVE_GLES: Enable OpenGL ES support for GPU rendering
CFLAGS += -mfloat-abi=hard -march=armv8-a -mfpu=neon-vfpv4 -O2 -fno-pie
CFLAGS += -I/usr/include/arm-linux-gnueabihf
CFLAGS += -I/usr/include/SDL2
CFLAGS += -DSDL_VIDEO_DRIVER_WAYLAND -USDL_VIDEO_DRIVER_X11 -DHAVE_GLES
CFLAGS += -D_REENTRANT -D_FILE_OFFSET_BITS=64 -Wno-unused-result

ASFLAGS += -mfpu=neon

# LDFLAGS - allow undefined symbols in shared libs (resolved on PSC at runtime)
LDFLAGS += -mfloat-abi=hard -no-pie -s -static-libgcc
LDFLAGS += -L/usr/lib/arm-linux-gnueabihf
LDFLAGS += -Wl,--allow-shlib-undefined -Wl,--hash-style=sysv

MAIN_LDFLAGS += -Wl,--allow-shlib-undefined

# Libraries - Wayland/EGL/GLES for fullscreen on PSC
MAIN_LDLIBS += -lSDL2 -lpng -ldl -lm -lpthread -lz
MAIN_LDLIBS += -Wl,--no-as-needed -lwayland-client -lwayland-egl -lEGL -lGLESv1_CM

PLUGIN_CFLAGS += -fPIC

# Architecture settings
ARCH = arm
PLATFORM = generic
# Use neon GPU for hardware-accelerated rendering
BUILTIN_GPU = neon
SOUND_DRIVERS = sdl
PLUGINS = plugins/spunull/spunull.so plugins/dfxvideo/gpu_peops.so plugins/gpu_unai/gpu_unai.so

# ARM features
HAVE_NEON = 1
HAVE_NEON_ASM = 1
# Enable ari64 dynarec (native ARM JIT compiler for speed)
DYNAREC = ari64

# OpenGL ES for Wayland fullscreen
HAVE_GLES = 1
