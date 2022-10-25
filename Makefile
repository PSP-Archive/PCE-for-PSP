BINARY = out

OBJECTS = \
	startup.o \
	main.o pg.o _clib.o menu.o  M6502.o sound.o \
	mp3.o clipmp3.o clipmp3a.o frammp3.o \
	pce_main.o pce_sound.o \

DEFINES = 

LIBRARY = unziplib.a iyanbakan.o

all: $(BINARY)

$(BINARY): $(OBJECTS)
	ld -O0 $(OBJECTS) $(LIBRARY) -M -Ttext 8900000 -q -o $@ > main.map
	outpatch
#	strip outp
	elf2pbp outp "PCE for PSP 0.7"

%.o : %.c
	gcc -march=r4000 -O2 -fomit-frame-pointer $(DEFINES) -g -mgp32 -mlong32 \
		-c $< -o $@

%.o : %.s
	gcc  -march=r4000 -g -mgp32 -c -xassembler -O -o $@ $<

clean:
	del /s /f *.o *.map
