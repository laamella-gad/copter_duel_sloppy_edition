OSCAR64 ?= oscar64
X64 ?= x64sc
PUCRUNCH ?= pucrunch

all: copterduel.prg

# src/main.c includes the other sources
build/copterduel_raw.prg: src/*.c src/*.h src/*.bin src/*.sid
	mkdir -p build
	$(OSCAR64) -n -O2 -o=$@ src/main.c

# The released PRG, checked in. Starts at the oscar64 startup code after the
# BASIC SYS line ($080d)
copterduel.prg: build/copterduel_raw.prg
	$(PUCRUNCH) -c64 -x0x080d $< $@

run: copterduel.prg
	$(X64) -sound $<

clean:
	rm -rf build

.PHONY: all run clean
