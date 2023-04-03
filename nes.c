#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "libretro.h"
#include "u.h"
#include "compat.h"
#include "dat.h"
#include "fns.h"

static retro_input_state_t input_state_cb;
static retro_input_poll_t input_poll_cb;
static retro_video_refresh_t video_cb;
static retro_environment_t environ_cb;
retro_audio_sample_batch_t audio_cb;

extern uchar ppuram[16384];
int nprg, nchr, map, chrram;
uchar *prg, *chr;
int cpuclock, ppuclock, apuclock, dmcclock, dmcfreq, sampclock, msgclock, saveclock = 0;
int oflag, savefd = -1;
int mirr;
int doflush = 0;
uchar *pic;
u16int keys[2];

void
flushram(void)
{
	//if(savefd >= 0)
	//	pwrite(savefd, mem + 0x6000, 0x2000, 0);
	saveclock = 0;
}

void
loadrom(const void *data)
{
	int nes20;
	static uchar header[16];
	static u32int flags;
	static char buf[512];

	memcpy(header, data, sizeof(header));
	if(memcmp(header, "NES\x1a", 4) != 0)
		sysfatal("not a ROM");
	if(header[15] != 0)
		memset(header + 7, 0, 9);
	flags = header[6] | header[7] << 8;
	nes20 = (flags & FLNES20M) == FLNES20V;
	if(flags & (FLVS | FLPC10))
		sysfatal("ROM not supported");
	nprg = header[HPRG];
	if(nes20)
		nprg |= (header[HROMH] & 0xf) << 8;
	if(nprg == 0)
		sysfatal("invalid ROM");
	nchr = header[HCHR];
	if(nes20)
		nchr |= (header[HROMH] & 0xf0) << 4;
	map = (flags >> FLMAPPERL) & 0x0f | (((flags >> FLMAPPERH) & 0x0f) << 4);
	if(nes20)
		map |= (header[8] & 0x0f) << 8;
	if(map >= 256 || mapper[map] == nil)
		sysfatal("unimplemented mapper %d", map);

	memset(mem, 0, sizeof(mem));
	if((flags & FLTRAINER) != 0)
		memcpy(mem + 0x7000, data+sizeof(header), 512);
	prg = malloc(nprg * PRGSZ);
	if(prg == nil)
		sysfatal("malloc");
	memcpy(prg, data+sizeof(header), nprg * PRGSZ);
	chrram = nchr == 0;
	if(nchr != 0){
		chr = malloc(nchr * CHRSZ);
		if(chr == nil)
			sysfatal("malloc");
		memcpy(chr, data+sizeof(header)+(nprg * PRGSZ), nchr * CHRSZ);
	}else{
		nchr = 1;
		chr = malloc(nchr * CHRSZ);
		if(chr == nil)
			sysfatal("malloc");
	}
	if((flags & FLFOUR) != 0)
		mirr = MFOUR;
	else if((flags & FLMIRROR) != 0)
		mirr = MVERT;
	else
		mirr = MHORZ;
	mapper[map](INIT, 0);
}

int t;

void
retro_init(void)
{
}

void
retro_get_system_info(struct retro_system_info *info)
{
	memset(info, 0, sizeof(*info));
	info->library_name = "nes";
	info->library_version = "1.0";
	info->need_fullpath = false;
	info->valid_extensions = "nes";
}

void
retro_get_system_av_info(struct retro_system_av_info *info)
{
	info->timing.fps = 60.0;
	info->timing.sample_rate = 44100.0;

	info->geometry.base_width = 256;
	info->geometry.base_height = 240;
	info->geometry.max_width = 256;
	info->geometry.max_height = 240;
	info->geometry.aspect_ratio = 4.0 / 3.0;
}

unsigned
retro_api_version(void)
{
	return RETRO_API_VERSION;
}

bool
retro_load_game(const struct retro_game_info *game)
{
	enum retro_pixel_format fmt = RETRO_PIXEL_FORMAT_XRGB8888;
	if (!environ_cb(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT, &fmt))
		return false;

	pic = malloc(256 * 240 * 4);
	initaudio();
	loadrom(game->data);
	pc = memread(0xFFFC) | memread(0xFFFD) << 8;
	rP = FLAGI;
	dmcfreq = 12 * 428;
	return true;
}

static const int retro_bind[] = {
	[RETRO_DEVICE_ID_JOYPAD_B] = 1<<1, // B
	[RETRO_DEVICE_ID_JOYPAD_Y] = 0, // NOTHING
	[RETRO_DEVICE_ID_JOYPAD_SELECT] = 1<<2, // CONTROL
	[RETRO_DEVICE_ID_JOYPAD_START] = 1<<3, // START
	[RETRO_DEVICE_ID_JOYPAD_UP] = 1<<4, // UP
	[RETRO_DEVICE_ID_JOYPAD_DOWN] = 1<<5, // DOWN
	[RETRO_DEVICE_ID_JOYPAD_LEFT] = 1<<6, // LEFT
	[RETRO_DEVICE_ID_JOYPAD_RIGHT] = 1<<7, // RIGHT
	[RETRO_DEVICE_ID_JOYPAD_A] = 1<<0, // BUTTON_B
};

void
process_inputs()
{
	for(int p = 0; p < 2; p++)
	{
		keys[p] = 0;
		for(int id = 0; id < RETRO_DEVICE_ID_JOYPAD_X; id++)
			if(input_state_cb(p, RETRO_DEVICE_JOYPAD, 0, id))
				keys[p] ^= retro_bind[id];
	}
}

void
retro_run(void)
{
	input_poll_cb();
	process_inputs();

	while(!doflush){
		t = cpustep() * 12;
		cpuclock += t;
		ppuclock += t;
		apuclock += t;
		sampclock += t;
		dmcclock += t;
		while(ppuclock >= 4){
			ppustep();
			ppuclock -= 4;
		}
		if(apuclock >= APUDIV){
			apustep();
			apuclock -= APUDIV;
		}
		if(sampclock >= SAMPDIV){
			audiosample();
			sampclock -= SAMPDIV;
		}
		if(dmcclock >= dmcfreq){
			dmcstep();
			dmcclock -= dmcfreq;
		}
		if(saveclock > 0){
			saveclock -= t;
			if(saveclock <= 0)
				flushram();
		}
	}
	video_cb(pic, 256, 240, 256*4);
	audioout();
	doflush = 0;
}

void
flush(void)
{
	doflush = 1;
}

void
retro_set_input_poll(retro_input_poll_t cb)
{
	input_poll_cb = cb;
}

void
retro_set_input_state(retro_input_state_t cb)
{
	input_state_cb = cb;
}

void
retro_set_video_refresh(retro_video_refresh_t cb)
{
	video_cb = cb;
}

void
retro_set_environment(retro_environment_t cb)
{
	environ_cb = cb;
}

void
retro_set_audio_sample_batch(retro_audio_sample_batch_t cb)
{
    audio_cb = cb;
}

void
retro_reset(void)
{
	cpuclock = ppuclock = apuclock = dmcclock = sampclock = msgclock = saveclock = 0;
	doflush = 0;
	keys[0] = keys[1] = 0;
	initaudio();
	pc = memread(0xFFFC) | memread(0xFFFD) << 8;
	rP = FLAGI;
	dmcfreq = 12 * 428;
}

size_t
retro_serialize_size(void)
{
	return (32+16)*1024+256+40*8;
}

bool
retro_serialize(void *data, size_t size)
{
	return savestate(data, size);
}

bool
retro_unserialize(const void *data, size_t size)
{
	return loadstate(data, size);
}

void retro_set_controller_port_device(unsigned port, unsigned device) {}
size_t retro_get_memory_size(unsigned id) { return 0; }
void * retro_get_memory_data(unsigned id) { return NULL; }
void retro_unload_game(void) {}
void retro_deinit(void) {}
void retro_set_audio_sample(retro_audio_sample_t cb) {}
void retro_cheat_reset(void) {}
void retro_cheat_set(unsigned index, bool enabled, const char *code) {}
bool retro_load_game_special(unsigned game_type, const struct retro_game_info *info, size_t num_info) { return false; }
unsigned retro_get_region(void) { return 0; }
