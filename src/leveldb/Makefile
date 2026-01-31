# LevelDB Makefile wrapper for CMake build
# Used by iocoind build system

.PHONY: all libleveldb.a libmemenv.a clean

all: libleveldb.a libmemenv.a

libleveldb.a libmemenv.a:
	@if ! command -v cmake >/dev/null 2>&1; then \
		echo "Error: cmake not found. Install with: brew install cmake"; \
		exit 1; \
	fi
	@mkdir -p build
	@cd build && cmake -DCMAKE_BUILD_TYPE=Release .. >/dev/null 2>&1
	@cd build && $(MAKE) --no-print-directory
	@cp build/libleveldb.a .
	@if [ -f build/libmemenv.a ]; then \
		cp build/libmemenv.a .; \
	else \
		echo "Note: Modern LevelDB includes memenv in libleveldb.a"; \
		touch libmemenv.a; \
	fi
	@echo "LevelDB libraries built successfully"

clean:
	rm -rf build libleveldb.a libmemenv.a
