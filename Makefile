OSCAR64 ?= oscar64

all: build/copterduel_c.prg

# The C version: src/copterduel.c includes the other sources
build/copterduel_c.prg: src/*.c src/*.h build/sprites.bin build/charset.bin
	$(OSCAR64) -n -O2 -o=$@ src/copterduel.c

build/sprites.bin build/charset.bin: copterduel.vsf tools/extract_assets.py
	tools/extract_assets.py copterduel.vsf build

# The patched original
build/copterduel.prg: copterduel.vsf tools/build.py
	tools/build.py copterduel.vsf $@

clean:
	rm -rf build

.PHONY: all clean
