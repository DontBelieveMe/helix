#include "regalloc2.h"
#include "function.h"
#include "print.h"

using namespace Helix;

void RegisterAllocator2::Execute(Function* function)
{
	function->RunLivenessAnalysis();

	SlotTracker slots;
	slots.CacheFunction(function);

	for (const BasicBlock& bb : function->blocks()) {
		helix_info(logs::regalloc2, "bb{}", slots.GetBasicBlockSlot(&bb));

		const auto in = bb.GetLiveIn();
		const auto out = bb.GetLiveOut();

		helix_info(logs::regalloc2, "\tIN [count={}]:", in.size());
		for (const VirtualRegisterName* v : in) {
			helix_info(logs::regalloc2, "\t - %{}", slots.GetValueSlot(v));
		}

		helix_info(logs::regalloc2, "\tOUT [count={}]:", out.size());
		for (const VirtualRegisterName* v : out) {
			helix_info(logs::regalloc2, "\t - %{}", slots.GetValueSlot(v));
		}
	}
}