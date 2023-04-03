#include <fcntl.h>
#include <unistd.h>
#include <stdbool.h>
#include "u.h"
#include "compat.h"
#include "dat.h"
#include "fns.h"

static FILE* fp;

void
put8(u8int i)
{
	fwrite(&i, 1, 1, fp);
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
	
	fread(&c, 1, 1, fp);
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
	fp = fmemopen((void*)data, size, "rb");
	if(!fp){
		return false;
	}
	fread(mem, sizeof(mem), 1, fp);
	fread(ppuram, sizeof(ppuram), 1, fp);
	fread(oam, sizeof(oam), 1, fp);
	if(chrram)
		fread(chr, nchr * CHRSZ, 1, fp);
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
	keylatch[0] = get8();
	keylatch[1] = get8();
	vrambuf = get8();
	cpuclock = get32();
	ppuclock = get32();
	apuclock = get32();
	apuseq = get8();
	dmcaddr = get16();
	dmccnt = get16();
	fread(apuctr, sizeof(apuctr), 1, fp);
	mapper[map](RSTR, 0);
	fclose(fp);
	return true;
}

bool
savestate(void *data, size_t size)
{
	fp = fmemopen(data, size, "wb");
	if(!fp){
		return false;
	}
	fwrite(mem, sizeof(mem), 1, fp);
	fwrite(ppuram, sizeof(ppuram), 1, fp);
	fwrite(oam, sizeof(oam), 1, fp);
	if(chrram)
		fwrite(chr, nchr * CHRSZ, 1, fp);
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
	put8(keylatch[0]);
	put8(keylatch[1]);
	put8(vrambuf);
	put32(cpuclock);
	put32(ppuclock);
	put32(apuclock);
	put8(apuseq);
	put16(dmcaddr);
	put16(dmccnt);
	fwrite(apuctr, sizeof(apuctr), 1, fp);
	mapper[map](SAVE, 0);
	fclose(fp);
	return true;
}
