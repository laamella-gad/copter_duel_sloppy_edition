OSCAR64 ?= oscar64

all: build/copterduel.prg

# src/copterduel.c includes the other sources
build/copterduel.prg: src/*.c src/*.h src/*.bin
	mkdir -p build
	$(OSCAR64) -n -O2 -o=$@ src/copterduel.c

clean:
	rm -rf build

.PHONY: all clean
