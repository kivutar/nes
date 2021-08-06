SHARED := -dynamiclib
TARGET := nes_libretro.dylib
CFLAGS += -O3

OBJ = cpu.o mem.o ppu.o state.o apu.o nes.o compat.o

%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS)

$(TARGET): $(OBJ)
	$(CC) $(SHARED) -fPIC -o $@ $^ $(CFLAGS)

clean:
	rm *.o nes_libretro.*
