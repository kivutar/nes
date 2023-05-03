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

int
get8(void)
{
	u8int c;

	c = *(u8int*)addr; addr += 1;
	return c;
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
	rA = *(u8int*)addr; addr += 1;
	rX = *(u8int*)addr; addr += 1;
	rY = *(u8int*)addr; addr += 1;
	rS = *(u8int*)addr; addr += 1;
	rP = *(u8int*)addr; addr += 1;
	nmi = *(u8int*)addr; addr += 1;
	pc = *(u16int*)addr; addr += 2;
	pput = *(u16int*)addr; addr += 2;
	ppuv = *(u16int*)addr; addr += 2;
	ppusx = *(u8int*)addr; addr += 1;
	ppux = *(u16int*)addr; addr += 2;
	ppuy = *(u16int*)addr; addr += 2;
	mirr = *(u8int*)addr; addr += 1;
	odd = *(u8int*)addr; addr += 1;
	vramlatch = *(u8int*)addr; addr += 1;
	keylatch[0] = *(u32int*)addr; addr += 4;
	keylatch[1] = *(u32int*)addr; addr += 4;
	vrambuf = *(u8int*)addr; addr += 1;
	cpuclock = *(u32int*)addr; addr += 4;
	ppuclock = *(u32int*)addr; addr += 4;
	apuclock = *(u32int*)addr; addr += 4;
	apuseq = *(u8int*)addr; addr += 1;
	dmcaddr = *(u16int*)addr; addr += 2;
	dmccnt = *(u16int*)addr; addr += 2;
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
	*(u8int*)addr = rA; addr += 1;
	*(u8int*)addr = rX; addr += 1;
	*(u8int*)addr = rY; addr += 1;
	*(u8int*)addr = rS; addr += 1;
	*(u8int*)addr = rP; addr += 1;
	*(u8int*)addr = nmi; addr += 1;
	*(u16int*)addr = pc; addr += 2;
	*(u16int*)addr = pput; addr += 2;
	*(u16int*)addr = ppuv; addr += 2;
	*(u8int*)addr = ppusx; addr += 1;
	*(u16int*)addr = ppux; addr += 2;
	*(u16int*)addr = ppuy; addr += 2;
	*(u8int*)addr = mirr; addr += 1;
	*(u8int*)addr = odd; addr += 1;
	*(u8int*)addr = vramlatch; addr += 1;
	*(u32int*)addr = keylatch[0]; addr += 4;
	*(u32int*)addr = keylatch[1]; addr += 4;
	*(u8int*)addr = vrambuf; addr += 1;
	*(u32int*)addr = cpuclock; addr += 4;
	*(u32int*)addr = ppuclock; addr += 4;
	*(u32int*)addr = apuclock; addr += 4;
	*(u8int*)addr = apuseq; addr += 1;
	*(u16int*)addr = dmcaddr; addr += 2;
	*(u16int*)addr = dmccnt; addr += 2;
	memcpy(addr, apuctr, sizeof(apuctr)); addr += sizeof(apuctr);
	mapper[map](SAVE, 0);
	return true;
}
