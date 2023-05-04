#include <string.h>
#include <stdio.h>
#include "hcdebug.h"
#include "libretro.h"
#include "u.h"
#include "dat.h"

extern int nprg;
extern uchar *prg;
extern uchar mem[32768];

extern u16int pc;
extern u8int rA, rX, rY, rS, rP;

uint8_t mem_peek(uint64_t address)
{
	return mem[address];
}

int mem_poke(uint64_t address, uint8_t value)
{
	mem[address] = value;
	return true;
}

hc_Memory const main_memory = {
	/* id, description*/
	"cpu", "Main",
	/* alignment, base_address, size */
	1, 0, 0x10000,

	// breakpoints, num_breakpoints
	NULL, 0,

	/* peek */
	mem_peek,

	/* poke */
	mem_poke,
};

uint8_t prg_peek(uint64_t address)
{
	uint8_t* data = prg;
	return (data && address < nprg * PRGSZ)
			? *(data + address)
			: 0;
}

int prg_poke(uint64_t address, uint8_t value)
{
	uint8_t* data = prg;
	if (data && address < nprg * PRGSZ)
	{
		*data = value;
		return true;
	}
	else
	{
		return false;
	}
}

hc_Memory prg_rom = {
	"rom", "prg ROM",

	/* alignment, base_address, size */
	1, 0, 0,

	/* break_points, num_breakpoints */
	NULL, 0,

	/* peek */
	prg_peek,

	/* poke */
	prg_poke,
};

uint64_t get_register(unsigned reg)
{
	printf("get_register(%u)\n", reg);
	switch (reg)
	{
	case HC_6502_A:
		return rA;
	case HC_6502_X:
		return rX;
	case HC_6502_Y:
		return rY;
	case HC_6502_S:
		return rS;
	case HC_6502_PC:
		return pc;
	case HC_6502_P:
		return rP;
	default:
		return 0;
	}
}

int set_register(unsigned reg, uint64_t value)
{
	printf("set_register(%u, %llu)\n", reg, value);
	switch (reg)
	{
	case HC_6502_A:
		rA = value;
		break;
	case HC_6502_X:
		rX =value;
		break;
	case HC_6502_Y:
		rY = value;
		break;
	case HC_6502_S:
		rS = value;
		break;
	case HC_6502_PC:
		pc = value;
		return 1;
	case HC_6502_P:
		rP = value;
		break;
	}
// 		cpu->SetState(state);
	return 1;
}

hc_Cpu const cpu = {
	/* id, description, type, is_main */
	"main-cpu", "Main CPU", HC_CPU_6502, 1,
	/* memory_region */
	&main_memory,
	/* break_points, num_break_points */
	NULL, 0,
	/* get_register */
	get_register,
	// /* set_register */
	set_register,
};

hc_Cpu const* cpus[] = {
	&cpu
};

hc_Memory const* system_memory[] = {
	&prg_rom
};

hc_System const nes_system = {
	/* description */
	"NES",
	/* cpus, num_cpus */
	cpus, sizeof(cpus) / sizeof(cpus[0]),
	/* memory_regions, num_memory_regions */
	system_memory, 1,
	/* break_points, num_break_points */
	NULL, 0
};

static RETRO_CALLCONV void* hc_set_debugger(hc_DebuggerIf* const debugger_if) {
	hc_DebuggerIf* debugger = debugger_if;
	debugger_if->core_api_version = HC_API_VERSION;
	debugger_if->v1.system = &nes_system;
	// debugger_if->v1.subscribe = subscribe;
	// debugger_if->v1.unsubscribe = unsubscribe;

	// if (_console && _console->GetMapper())
	// {
		prg_rom.v1.size = nprg * PRGSZ;
	// }

	return (void*)&nes_system;
}

RETRO_CALLCONV retro_proc_address_t get_proc_address(const char* sym)
{
	if (!strcmp(sym, "hc_set_debuggger") || !strcmp(sym, "hc_set_debugger"))
	{
		return (retro_proc_address_t)hc_set_debugger;
	}
	
	return NULL;
}
