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
int clock, ppuclock, apuclock, dmcclock, dmcfreq, sampclock, msgclock, saveclock;
int oflag, savefd = -1;
int mirr;
int doflush = 0;

int
readn(int f, void *data, int len)
{
	uchar *p, *e;

	p = data;
	e = p + len;
	while(p < e){
		if((len = read(f, p, e - p)) <= 0)
			break;
		p += len;
	}
	return p - (uchar*)data;
}

/*void
message(char *fmt, ...)
{
	va_list va;
	static char buf[512];
	
	va_start(va, fmt);
	vsnprint(buf, sizeof buf, fmt, va);
	string(screen, Pt(10, 10), display->black, ZP, display->defaultfont, buf);
	msgclock = FREQ;
	va_end(va);
}*/

void
flushram(void)
{
	if(savefd >= 0)
		pwrite(savefd, mem + 0x6000, 0x2000, 0);
	saveclock = 0;
}

void
loadrom(const char *file, int sflag)
{
	int fd;
	int nes20;
	char *s;
	static uchar header[16];
	static u32int flags;
	static char buf[512];
	
	fd = open(file, OREAD);
	if(fd < 0)
		sysfatal("open\n");
	if(readn(fd, header, sizeof(header)) < sizeof(header))
		sysfatal("read\n");
	if(memcmp(header, "NES\x1a", 4) != 0)
		sysfatal("not a ROM\n");
	if(header[15] != 0)
		memset(header + 7, 0, 9);
	flags = header[6] | header[7] << 8;
	nes20 = (flags & FLNES20M) == FLNES20V;
	if(flags & (FLVS | FLPC10))
		sysfatal("ROM not supported\n");
	nprg = header[HPRG];
	if(nes20)
		nprg |= (header[HROMH] & 0xf) << 8;
	if(nprg == 0)
		sysfatal("invalid ROM\n");
	nchr = header[HCHR];
	if(nes20)
		nchr |= (header[HROMH] & 0xf0) << 4;
	map = (flags >> FLMAPPERL) & 0x0f | (((flags >> FLMAPPERH) & 0x0f) << 4);
	if(nes20)
		map |= (header[8] & 0x0f) << 8;
	if(map >= 256 || mapper[map] == nil)
		sysfatal("unimplemented mapper %d\n", map);

	memset(mem, 0, sizeof(mem));
	if((flags & FLTRAINER) != 0 && readn(fd, mem + 0x7000, 512) < 512)
			sysfatal("read\n");
	prg = malloc(nprg * PRGSZ);
	if(prg == nil)
		sysfatal("malloc\n");
	if(readn(fd, prg, nprg * PRGSZ) < nprg * PRGSZ)
		sysfatal("read\n");
	chrram = nchr == 0;
	if(nchr != 0){
		chr = malloc(nchr * CHRSZ);
		if(chr == nil)
			sysfatal("malloc\n");
		if(readn(fd, chr, nchr * CHRSZ) < nchr * CHRSZ)
			sysfatal("read\n");
	}else{
		nchr = 1;
		chr = malloc(nchr * CHRSZ);
		if(chr == nil)
			sysfatal("malloc\n");
	}
	if((flags & FLFOUR) != 0)
		mirr = MFOUR;
	else if((flags & FLMIRROR) != 0)
		mirr = MVERT;
	else
		mirr = MHORZ;
	/*if(sflag){
		strncpy(buf, file, sizeof buf - 5);
		s = buf + strlen(buf) - 4;
		if(s < buf || strcmp(s, ".nes") != 0)
			s += 4;
		strcpy(s, ".sav");
		savefd = create(buf, ORDWR | OEXCL, 0666);
		if(savefd < 0)
			savefd = open(buf, ORDWR);
		if(savefd < 0)
			message("open: %r");
		else
			readn(savefd, mem + 0x6000, 0x2000);
		atexit(flushram);
	}*/
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
	info->need_fullpath = true;
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
	loadrom(game->path, 0);
	pc = memread(0xFFFC) | memread(0xFFFD) << 8;
	rP = FLAGI;
	dmcfreq = 12 * 428;
	return true;
}

void
retro_run(void)
{
	input_poll_cb();

	while(!doflush){
		if(savereq){
			savestate("nes.save");
			savereq = 0;
		}
		if(loadreq){
			loadstate("nes.save");
			loadreq = 0;
		}
		t = step() * 12;
		clock += t;
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
		/*if(msgclock > 0){
			msgclock -= t;
			if(msgclock <= 0){
				extern Image *bg;
				draw(screen, screen->r, bg, nil, ZP);
				msgclock = 0;
			}
		}*/
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

void retro_set_controller_port_device(unsigned port, unsigned device) {}
size_t retro_get_memory_size(unsigned id) { return 0; }
void * retro_get_memory_data(unsigned id) { return NULL; }
void retro_reset(void) {}
void retro_unload_game(void) {}
void retro_deinit(void) {}
void retro_set_audio_sample(retro_audio_sample_t cb) {}
size_t retro_serialize_size(void) { return 0; }
bool retro_serialize(void *data, size_t size) { return false; }
bool retro_unserialize(const void *data, size_t size) { return false; }
void retro_cheat_reset(void) {}
void retro_cheat_set(unsigned index, bool enabled, const char *code) {}
bool retro_load_game_special(unsigned game_type, const struct retro_game_info *info, size_t num_info) { return false; }
unsigned retro_get_region(void) { return 0; }
