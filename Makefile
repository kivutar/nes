SHARED := -shared
TARGET := nes_libretro.so

ifeq ($(shell uname -s),) # win
	SHARED := -shared
	TARGET := nes_libretro.dll
else ifneq ($(findstring MINGW,$(shell uname -s)),) # win
	SHARED := -shared
	TARGET := nes_libretro.dll
else ifneq ($(findstring Darwin,$(shell uname -s)),) # osx
	SHARED := -dynamiclib
	TARGET := nes_libretro.dylib
endif

CFLAGS += -O3 -fPIC -flto

OBJ = cpu.o mem.o ppu.o state.o apu.o nes.o compat.o

%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS)

$(TARGET): $(OBJ)
	$(CC) $(SHARED) -o $@ $^ $(CFLAGS)

clean:
	rm $(OBJ) $(TARGET)
