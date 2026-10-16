OSCAR64 ?= oscar64
X64 ?= x64sc
PUCRUNCH ?= pucrunch

all: build/copterduel_crunched.prg

# src/copterduel.c includes the other sources
build/copterduel.prg: src/*.c src/*.h src/*.bin
	mkdir -p build
	$(OSCAR64) -n -O2 -o=$@ src/copterduel.c

# Starts at the oscar64 startup code after the BASIC SYS line ($080d)
build/copterduel_crunched.prg: build/copterduel.prg
	$(PUCRUNCH) -c64 -x0x080d $< $@

run: build/copterduel_crunched.prg
	$(X64) -sound $<

clean:
	rm -rf build

.PHONY: all run clean
