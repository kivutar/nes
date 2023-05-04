#include <string.h>
#include <stdio.h>
#include "hcdebug.h"
#include "libretro.h"
#include "u.h"
#include "dat.h"

extern int nprg;
extern uchar *prg;
extern uchar mem[32768];

extern hc_Memory const main_memory;
extern hc_Cpu const cpu;

// class HCDebugContext : public ScriptingContext
// {
// public:
// 	HCDebugContext(Debugger* d)
// 		: ScriptingContext(d)
// 	{}
// protected:
// 	void InternalCallMemoryCallback(uint16_t addr, uint8_t& value, CallbackType type) override
// 	{
// 		hc_Event e;
// 		switch (type)
// 		{
// 		case CallbackType::CpuExec:
// 			e.type = HC_EVENT_EXECUTION;
// 			e.execution.cpu = &cpu;
// 			e.execution.address = addr;
// 			break;
// 		case CallbackType::CpuWrite:
// 		case CallbackType::CpuRead:
// 			e.type = HC_EVENT_MEMORY;
// 			e.memory.memory = &main_memory;
// 			e.memory.address = addr;
// 			e.memory.operation = (type == CallbackType::CpuRead)
// 				? HC_MEMORY_READ
// 				: HC_MEMORY_WRITE;
// 			e.memory.value = value;
// 			break;
// 		default:
// 			return;
// 		}
		
// 		for (auto& ref : _callbacks[(int)type][addr])
// 		{
// 			if (debugger->v1.handle_event)
// 			{
// 				debugger->v1.handle_event(debugger->v1.user_data, ref, &e);
// 			}
// 		}
// 	}
	
// 	int InternalCallEventCallback(EventType type) override
// 	{
// 		return 0;
// 	}

// public:
// 	uint64_t GetRegister(unsigned reg)
// 	{
// 		State state;
// 		_console->GetCpu()->GetState(state);
// 		switch (reg)
// 		{
// 		case HC_6502_A:
// 			return state.A;
// 		case HC_6502_X:
// 			return state.X;
// 		case HC_6502_Y:
// 			return state.Y;
// 		case HC_6502_S:
// 			return state.PS;
// 		case HC_6502_PC:
// 			return state.PC;
// 		case HC_6502_P:
// 			return state.SP;
// 		default:
// 			return 0;
// 		}
// 	}
	
// 	void SetRegister(unsigned reg, uint64_t value)
// 	{
// 		State state;
// 		CPU* cpu = _console->GetCpu();
// 		cpu->GetState(state);
// 		switch (reg)
// 		{
// 		case HC_6502_A:
// 			state.A = value;
// 			break;
// 		case HC_6502_X:
// 			state.X =value;
// 			break;
// 		case HC_6502_Y:
// 			state.Y = value;
// 			break;
// 		case HC_6502_S:
// 			state.PS = value;
// 			break;
// 		case HC_6502_PC:
// 			cpu->SetDebugPC(value);
// 			return;
// 		case HC_6502_P:
// 			state.SP = value;
// 			break;
// 		}
// 		cpu->SetState(state);
// 	}
	
// 	uint8_t peek(uint64_t address)
// 	{
// 		return _console->GetMemoryManager()->DebugRead(address);
// 	}
	
// 	void poke(uint64_t address, uint8_t value)
// 	{
// 		_console->GetMemoryManager()->DebugWrite(address, value);
// 	}
	
// 	void step()
// 	{
// 		GetDebugger()->ReadStepContext();
// 		GetDebugger()->Step();
// 	}
	
// 	void step_over()
// 	{
// 		GetDebugger()->ReadStepContext();
// 		GetDebugger()->StepOver();
// 	}
	
// 	void step_out()
// 	{
// 		GetDebugger()->ReadStepContext();
// 		GetDebugger()->StepOut();
// 	}
// };

// HCDebugContext& get_scripting_context()
// {
// 	static shared_ptr<HCDebugContext> context;
// 	if (context) return *context;
// 	shared_ptr<Debugger> debugger = _console->GetDebugger();
// 	context.reset(new HCDebugContext(debugger.get()));
// 	debugger->AttachScript(context);
// 	return *context;
// }

static hc_SubscriptionID breakpoint_id = 0;

hc_SubscriptionID next_breakpoint_id()
{
	return breakpoint_id = (breakpoint_id + 1) & ~(1ULL << 63ULL);
}

uint8_t mem_peek(uint64_t address)
{
	printf("mem_peek %04x\n", address);
	return mem[address];
}

int mem_poke(uint64_t address, uint8_t value)
{
	printf("mem_poke %04x %02x\n", address, value);
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
	printf("prg_peek %04x\n", address);
	uint8_t* data = prg;
	return (data && address < nprg * PRGSZ)
			? *(data + address)
			: 0;
}

int prg_poke(uint64_t address, uint8_t value)
{
	printf("prg_poke %04x %02x\n", address, value);
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
	return 0; // TODO
}

int set_register(unsigned reg, uint64_t value)
{
	printf("set_register(%u, %llu)\n", reg, value);
	return true; // TODO
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
