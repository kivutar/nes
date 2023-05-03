#include <fcntl.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#include "u.h"
#include "compat.h"
#include "dat.h"
#include "fns.h"

static void* addr;

void
put8(u8int i)
{
	*(u8int*)addr = i; addr += 1;
}

void
put16(u16int i)
{
	put8(i);
	put8(i >> 8);
}

void
put32(u32int i)
{
	put8(i);
	put8(i >> 8);
	put8(i >> 16);
	put8(i >> 24);
}

int
get8(void)
{
	u8int c;

	c = *(u8int*)addr; addr += 1;
	return c;
}

int
get16(void)
{
	int i;

	i = get8();
	i |= get8() << 8;
	return i;
}

int
get32(void)
{
       int i;
       
       i = get8();
       i |= get8() << 8;
       i |= get8() << 16;
       i |= get8() << 24;
       return i;
}

bool
loadstate(const void *data, size_t size)
{
	addr = data;
	memcpy(mem, addr, sizeof(mem)); addr += sizeof(mem);
	memcpy(ppuram, addr, sizeof(ppuram)); addr += sizeof(ppuram);
	memcpy(oam, addr, sizeof(oam)); addr += sizeof(oam);
	if(chrram){
		memcpy(chr, addr, nchr * CHRSZ); addr += nchr * CHRSZ;
	}
	rA = get8();
	rX = get8();
	rY = get8();
	rS = get8();
	rP = get8();
	nmi = get8();
	pc = get16();
	pput = get16();
	ppuv = get16();
	ppusx = get8();
	ppux = get16();
	ppuy = get16();
	mirr = get8();
	odd = get8();
	vramlatch = get8();
	keylatch[0] = get32();
	keylatch[1] = get32();
	vrambuf = get8();
	cpuclock = get32();
	ppuclock = get32();
	apuclock = get32();
	apuseq = get8();
	dmcaddr = get16();
	dmccnt = get16();
	memcpy(apuctr, addr, sizeof(apuctr)); addr += sizeof(apuctr);
	mapper[map](RSTR, 0);
	return true;
}

bool
savestate(void *data, size_t size)
{
	addr = data;
	memcpy(addr, mem, sizeof(mem)); addr += sizeof(mem);
	memcpy(addr, ppuram, sizeof(ppuram)); addr += sizeof(ppuram);
	memcpy(addr, oam, sizeof(oam)); addr += sizeof(oam);
	if(chrram){
		memcpy(addr, chr, nchr * CHRSZ); addr += nchr * CHRSZ;
	}
	put8(rA);
	put8(rX);
	put8(rY);
	put8(rS);
	put8(rP);
	put8(nmi);
	put16(pc);
	put16(pput);
	put16(ppuv);
	put8(ppusx);
	put16(ppux);
	put16(ppuy);
	put8(mirr);
	put8(odd);
	put8(vramlatch);
	put32(keylatch[0]);
	put32(keylatch[1]);
	put8(vrambuf);
	put32(cpuclock);
	put32(ppuclock);
	put32(apuclock);
	put8(apuseq);
	put16(dmcaddr);
	put16(dmccnt);
	memcpy(addr, apuctr, sizeof(apuctr)); addr += sizeof(apuctr);
	mapper[map](SAVE, 0);
	return true;
}
