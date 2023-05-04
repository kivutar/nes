#include <string.h>
#include "hcdebug.h"
#include "libretro.h"

hc_DebuggerIf* debugger = NULL;

extern hc_Memory const main_memory;
extern hc_Cpu const cpu;


class HCDebugContext : public ScriptingContext
{
public:
	HCDebugContext(Debugger* d)
		: ScriptingContext(d)
	{}
protected:
	void InternalCallMemoryCallback(uint16_t addr, uint8_t& value, CallbackType type) override
	{
		hc_Event e;
		switch (type)
		{
		case CallbackType::CpuExec:
			e.type = HC_EVENT_EXECUTION;
			e.execution.cpu = &cpu;
			e.execution.address = addr;
			break;
		case CallbackType::CpuWrite:
		case CallbackType::CpuRead:
			e.type = HC_EVENT_MEMORY;
			e.memory.memory = &main_memory;
			e.memory.address = addr;
			e.memory.operation = (type == CallbackType::CpuRead)
				? HC_MEMORY_READ
				: HC_MEMORY_WRITE;
			e.memory.value = value;
			break;
		default:
			return;
		}
		
		for (auto& ref : _callbacks[(int)type][addr])
		{
			if (debugger->v1.handle_event)
			{
				debugger->v1.handle_event(debugger->v1.user_data, ref, &e);
			}
		}
	}
	
	int InternalCallEventCallback(EventType type) override
	{
		return 0;
	}

public:
	uint64_t GetRegister(unsigned reg)
	{
		State state;
		_console->GetCpu()->GetState(state);
		switch (reg)
		{
		case HC_6502_A:
			return state.A;
		case HC_6502_X:
			return state.X;
		case HC_6502_Y:
			return state.Y;
		case HC_6502_S:
			return state.PS;
		case HC_6502_PC:
			return state.PC;
		case HC_6502_P:
			return state.SP;
		default:
			return 0;
		}
	}
	
	void SetRegister(unsigned reg, uint64_t value)
	{
		State state;
		CPU* cpu = _console->GetCpu();
		cpu->GetState(state);
		switch (reg)
		{
		case HC_6502_A:
			state.A = value;
			break;
		case HC_6502_X:
			state.X =value;
			break;
		case HC_6502_Y:
			state.Y = value;
			break;
		case HC_6502_S:
			state.PS = value;
			break;
		case HC_6502_PC:
			cpu->SetDebugPC(value);
			return;
		case HC_6502_P:
			state.SP = value;
			break;
		}
		cpu->SetState(state);
	}
	
	uint8_t peek(uint64_t address)
	{
		return _console->GetMemoryManager()->DebugRead(address);
	}
	
	void poke(uint64_t address, uint8_t value)
	{
		_console->GetMemoryManager()->DebugWrite(address, value);
	}
	
	void step()
	{
		GetDebugger()->ReadStepContext();
		GetDebugger()->Step();
	}
	
	void step_over()
	{
		GetDebugger()->ReadStepContext();
		GetDebugger()->StepOver();
	}
	
	void step_out()
	{
		GetDebugger()->ReadStepContext();
		GetDebugger()->StepOut();
	}
};

HCDebugContext& get_scripting_context()
{
	static shared_ptr<HCDebugContext> context;
	if (context) return *context;
	shared_ptr<Debugger> debugger = _console->GetDebugger();
	context.reset(new HCDebugContext(debugger.get()));
	debugger->AttachScript(context);
	return *context;
}

static hc_SubscriptionID breakpoint_id = 0;

hc_SubscriptionID next_breakpoint_id()
{
	return breakpoint_id = (breakpoint_id + 1) & ~(1ULL << 63ULL);
}

hc_Memory const main_memory = {
	/* id, description*/
	"cpu", "Main",
	/* alignment, base_address, size */
	1, 0, 0x10000,

	// breakpoints, num_breakpoints
	NULL, 0,

	/* peek */
	[](uint64_t address) -> uint8_t {
		return get_scripting_context().peek(address);
	},

	/* poke */
	[](uint64_t address, uint8_t value) -> int {
		get_scripting_context().poke(address, value);
		return true;
	},
};

hc_Memory prg_rom = {
	"rom", "prg ROM",

	/* alignment, base_address, size */
	1, 0, 0,

	/* break_points, num_breakpoints */
	NULL, 0,

	/* peek */
	[](uint64_t address) -> uint8_t {
		uint8_t* data = _console->GetMapper()->GetPrgRom();
		return (data && address < _console->GetMapper()->GetMemorySize(DebugMemoryType::PrgRom))
			? *(data + address)
			: 0;
	},

	/* poke */
	[](uint64_t address, uint8_t value) -> int {
		uint8_t* data = _console->GetMapper()->GetPrgRom();
		if (data && address < _console->GetMapper()->GetMemorySize(DebugMemoryType::PrgRom))
		{
			*data = value;
			return true;
		}
		else
		{
			return false;
		}
	},
};

hc_Cpu const cpu = {
	/* id, description, type, is_main */
	"main-cpu", "Main CPU", HC_CPU_6502, 1,
	/* memory_region */
	&main_memory,
	/* break_points, num_break_points */
	NULL, 0,
	/* get_register */
	[](unsigned reg) -> uint64_t {
		return get_scripting_context().GetRegister(reg);
	},
	/* set_register */
	[](unsigned reg, uint64_t value) -> int {
		get_scripting_context().SetRegister(reg, value);
		return true;
	},
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

static RETRO_CALLCONV retro_proc_address_t get_proc_address(const char* sym)
{
	if (!strcmp(sym, "hc_set_debuggger") || !strcmp(sym, "hc_set_debugger"))
	{
		return (retro_proc_address_t)hc_set_debugger;
	}
	
	return nullptr;
}

static RETRO_CALLCONV void* hc_set_debugger(hc_DebuggerIf* const debugger_if) {
	debugger = debugger_if;
	debugger_if->core_api_version = HC_API_VERSION;
	debugger_if->v1.system = &nes_system;
	// debugger_if->v1.subscribe = subscribe;
	// debugger_if->v1.unsubscribe = unsubscribe;

	// if (_console && _console->GetMapper())
	// {
		prg_rom.v1.size = _console->GetMapper()->GetMemorySize(DebugMemoryType::PrgRom);
	// }

	return (void*)&nes_system;
}
