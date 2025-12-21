PCSX-ReARMed - AutoBleem-NG PlayStation Classic Fork
====================================================

This is the [AutoBleem-NG](https://github.com/AutoBleem-NG) fork of PCSX-ReARMed,
optimized for the [PlayStation Classic](https://en.wikipedia.org/wiki/PlayStation_Classic).

### PSC-Specific Changes
* SDL2 with Wayland/EGL/GLES support for native PSC display
* PSC controller support (USB HID joystick buttons mapped to PS1 controls)
* PSC launcher argument compatibility (`-region`, `-filter`, `-ratio`, `-enter`)
* Docker-based cross-compilation toolchain (GCC 9, glibc 2.23, kernel 4.4)
* Select+Start combo to enter emulator menu

### Building for PSC

**Requirements:** Docker

```bash
# Clone with submodules
git clone --recursive https://github.com/AutoBleem-NG/pcsx_rearmed_psc.git

# Build (first build takes ~15 min for toolchain)
make docker

# Output binaries
ls pcsx_bin/           # pcsx-ab
ls pcsx_bin/plugins/   # gpu_peops.so, gpu_unai.so
```

---

## Original PCSX-ReARMed

PCSX ReARMed is yet another PCSX fork based on the PCSX-Reloaded project,
which itself contains code from PCSX, PCSX-df and PCSX-Revolution. This
version was originally ARM architecture oriented (hence the name) with
its MIPS->ARM dynamic recompilation and assembly optimizations, but more
recently it targets other architectures too.

Upstream repository: https://github.com/notaz/pcsx_rearmed

## Features
* ARM/ARM64 dynamic recompiler by Ari64
* [lightrec](https://github.com/pcercuei/lightrec/) dynamic recompiler for other architectures
* NEON GPU by Exophase for ARM NEON and x86 SSE2+
* PCSX4ALL GPU by Una-i/senquack for other architectures
* heavily modified P.E.Op.S. SPU
* BIOS HLE emulation (most games run without proprietary BIOS)
* libretro support

