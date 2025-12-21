# Makefile.psc - PCSX ReARMed build for PlayStation Classic
#
# Usage:
#   make -f Makefile.psc        - Build for PSC (via Docker recommended)
#   make -f Makefile.psc clean  - Clean build artifacts

.PHONY: all arm clean distclean

# Toolchain prefix (provided by Docker build environment)
CC_ARM := armv8-sony-linux-gnueabihf-gcc

# Parallel jobs
JOBS ?= $(shell nproc 2>/dev/null || echo 4)

# Default target
all: arm

# Build for ARM (PlayStation Classic)
arm:
	@if ! command -v $(CC_ARM) >/dev/null 2>&1; then \
		echo "ERROR: ARM toolchain not found. Use 'make docker' instead."; \
		exit 1; \
	fi
	@if [ ! -f config.mak ]; then \
		echo "Configuring for PSC..."; \
		cp config.mak.psc config.mak; \
		test -e skin || ln -s frontend/pandora/skin skin; \
	fi
	@echo "Building PCSX ReARMed for PSC..."
	$(MAKE) -j$(JOBS)

# Clean build artifacts
clean:
	$(MAKE) clean 2>/dev/null || true

# Full clean
distclean: clean
	rm -f config.mak config.log include/revision.h
