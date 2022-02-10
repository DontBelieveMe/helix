/* Internal Project Includes */
#include "regalloc2.h"
#include "function.h"
#include "print.h"
#include "ir-helpers.h"
#include "instruction-index.h"
#include "interval.h"

/* C Standard Library Includes */
#include <ctype.h>

/* C++ Standard Library Includes */
#include <algorithm>

#pragma optimize("", off)

using namespace Helix;

/*********************************************************************************************************************/

template <typename MapType, typename KeyType>
static bool Contains(const MapType& assoc, const KeyType& key)
{
	auto it = assoc.find(key);
	return it != assoc.end();
}

/*********************************************************************************************************************/

static bool InstructionHasOperandWithFlag(const Instruction* insn, VirtualRegisterName* vreg, Instruction::OperandFlags flag)
{
	for (size_t i = 0; i < insn->GetCountOperands(); ++i) {
		if (insn->GetOperand(i) == vreg && insn->OperandHasFlags(i, flag))
			return true;
	}

	return false;
}

/*********************************************************************************************************************/

static size_t NewStackLocation(size_t* stackCounter)
{
	const size_t tmp = *stackCounter;
	*stackCounter = *stackCounter + 1;
	return tmp;
}

/*********************************************************************************************************************/

static void SpillAtInterval(size_t* stackCounter, std::vector<Interval>* active, Interval* interval)
{
	Interval& spill = active->back();

	if (!IntervalEndComparator()(spill, *interval)) {
		interval->physical_register = spill.physical_register;
		spill.stack_slot = NewStackLocation(stackCounter);

		active->erase(std::remove(active->begin(), active->end(), spill), active->end());
		active->push_back(*interval);

		std::sort(active->begin(), active->end(), IntervalEndComparator());
	}
	else {
		interval->stack_slot = NewStackLocation(stackCounter);
	}
}

/*********************************************************************************************************************/

static void ExpireOldIntervals(std::set<PhysicalRegisterName*>* free_regs, std::vector<Interval>* active, Interval* interval)
{
	std::vector<Interval> keep;

	for (const Interval& active_interval : *active) {
		if (!IntervalEndStartComparator()(active_interval, *interval)) {
			keep.push_back(active_interval);
			continue;
		}
		
		free_regs->insert(active_interval.physical_register);
	}

	*active = keep;
	std::sort(active->begin(), active->end(), IntervalEndComparator());
}

/*********************************************************************************************************************/

void RegisterAllocator2::Execute(Function* function)
{
	function->RunLivenessAnalysis();

	/* debug log liveness information */
	SlotTracker slots;
	slots.CacheFunction(function);

	//Helix::TextOutputStream tout(stdout);
	//Helix::Print(slots, tout, *function);

#if 1
	helix_debug(logs::regalloc2, "********** Liveness Analysis **********");

	for (const BasicBlock& bb : function->blocks()) {
		helix_debug(logs::regalloc2, "bb{}", slots.GetBasicBlockSlot(&bb));

		const auto in = bb.GetLiveIn();
		const auto out = bb.GetLiveOut();

		helix_debug(logs::regalloc2, "\tIN [count={}]:", in.size());
		for (const VirtualRegisterName* v : in) {
			helix_debug(logs::regalloc2, "\t   - %{}", slots.GetValueSlot(v));
		}

		helix_debug(logs::regalloc2, "\tOUT [count={}]:", out.size());
		for (const VirtualRegisterName* v : out) {
			helix_debug(logs::regalloc2, "\t   - %{}", slots.GetValueSlot(v));
		}
	}
#endif

	std::unordered_map<VirtualRegisterName*, Interval> intervals;

	{
		size_t blockIndex = 0;
		for (BasicBlock& bb : function->blocks()) {
			size_t instructionIndex;
			std::unordered_map<VirtualRegisterName*, Interval> uses;

			const std::set<VirtualRegisterName*>& block_in = bb.GetLiveIn();
			const std::set<VirtualRegisterName*>& block_out = bb.GetLiveOut();

			instructionIndex = 0;
			for (Instruction& insn : bb) {
				for (size_t opIndex = 0; opIndex < insn.GetCountOperands(); ++opIndex) {
					if (insn.OperandHasFlags(opIndex, Instruction::OP_READ)) {
						VirtualRegisterName* vreg = value_cast<VirtualRegisterName>(insn.GetOperand(opIndex));

						if (!vreg)
							continue;

						if (!Contains(block_in, vreg) && !Contains(block_out, vreg)) {
							Interval new_interval(vreg);
							new_interval.end = InstructionIndex(blockIndex, instructionIndex);
							new_interval.virtual_register = vreg;

							uses[vreg] = new_interval;
						}
					}
				}

				instructionIndex++;
			}

			instructionIndex = 0;
			for (Instruction& insn : bb) {
				for (size_t opIndex = 0; opIndex < insn.GetCountOperands(); ++opIndex) {
					if (insn.OperandHasFlags(opIndex, Instruction::OP_WRITE)) {
						VirtualRegisterName* vreg = value_cast<VirtualRegisterName>(insn.GetOperand(opIndex));

						if (!vreg)
							continue;

						if (!Contains(uses, vreg))
							continue;

						if (!Contains(block_in, vreg) && !Contains(block_out, vreg)) {
							uses[vreg].start = InstructionIndex(blockIndex, instructionIndex);
							intervals[vreg] = uses[vreg];
							uses.erase(vreg);
						}
					}
				}

				instructionIndex++;
			}

			for (VirtualRegisterName* vreg : block_in) {
				if (!Contains(intervals, vreg)) {
					Interval new_interval(vreg);
					new_interval.start = InstructionIndex(blockIndex, 0);

					intervals[vreg] = new_interval;
				}

				if (!Contains(block_out, vreg)) {
					InstructionIndex end = InstructionIndex(blockIndex, SIZE_MAX);

					instructionIndex = 0;
					for (Instruction& insn : bb) {
						if (InstructionHasOperandWithFlag(&insn, vreg, Instruction::OP_READ)) {
							end.instruction_index = instructionIndex;
						}

						instructionIndex++;
					}

					intervals[vreg].end = end;
				}
			}

			for (VirtualRegisterName* vreg : block_out) {
				if (!Contains(block_in, vreg) && !Contains(intervals, vreg)) {
					InstructionIndex start = InstructionIndex(blockIndex, SIZE_MAX);

					instructionIndex = 0;
					for (Instruction& insn : bb) {
						if (InstructionHasOperandWithFlag(&insn, vreg, Instruction::OP_WRITE)) {
							start.instruction_index = instructionIndex;
							break;
						}

						instructionIndex++;
					}

					Interval interval(vreg);
					interval.start = start;

					intervals[vreg] = interval;
				}
				else {
					InstructionIndex end = InstructionIndex(blockIndex, SIZE_MAX);

					instructionIndex = 0;
					for (Instruction& insn : bb) {
						if (InstructionHasOperandWithFlag(&insn, vreg, Instruction::OP_READ)) {
							end.instruction_index = instructionIndex;
						}

						instructionIndex++;
					}

					intervals[vreg].end = end;
				}
			}

			blockIndex++;
		}
	}

#if 1
	helix_debug(logs::regalloc2, "********** Interval Analysis **********");

	for (const auto [vreg, interval] : intervals) {
		helix_debug(logs::regalloc2, "%{} = {}:{} -> {}:{}",
			slots.GetValueSlot(vreg),
			interval.start.block_index, interval.start.instruction_index,
			interval.end.block_index, interval.end.instruction_index);
	}
#endif

	std::vector<Interval> intervals_sorted;
	
	{

		for (const auto& [vreg, interval] : intervals) {
			intervals_sorted.push_back(interval);
		}

		std::sort(intervals_sorted.begin(), intervals_sorted.end(), IntervalStartComparator());

		std::vector<Interval> active;
		size_t stackCounter = 0;

		std::set<PhysicalRegisterName*> free_registers =
		{
			PhysicalRegisters::GetRegister(BuiltinTypes::GetInt32(), PhysicalRegisters::R4),
			PhysicalRegisters::GetRegister(BuiltinTypes::GetInt32(), PhysicalRegisters::R5),
			PhysicalRegisters::GetRegister(BuiltinTypes::GetInt32(), PhysicalRegisters::R6),
			PhysicalRegisters::GetRegister(BuiltinTypes::GetInt32(), PhysicalRegisters::R7),
			PhysicalRegisters::GetRegister(BuiltinTypes::GetInt32(), PhysicalRegisters::R8),
		};

		const size_t kTotalAvailableRegisters = free_registers.size();

		for (Interval& interval : intervals_sorted) {
			ExpireOldIntervals(&free_registers, &active, &interval);

			if (active.size() >= kTotalAvailableRegisters) {
				SpillAtInterval(&stackCounter, &active, &interval);
			}
			else {
				PhysicalRegisterName* preg = *free_registers.begin();
				free_registers.erase(preg);

				interval.physical_register = preg;
				active.push_back(interval);
				std::sort(active.begin(), active.end(), IntervalEndComparator());
			}
		}
	}

#if 1
	helix_debug(logs::regalloc2, "********** Final Allocation **********");

	for (const Interval& interval : intervals_sorted)
	{
		helix_debug(logs::regalloc2, "%{} = {}:{} -> {}:{} (reg={}, stack={})",
			slots.GetValueSlot(interval.virtual_register),
			interval.start.block_index, interval.start.instruction_index,
			interval.end.block_index, interval.end.instruction_index,
			stringify_operand(interval.physical_register, slots), interval.stack_slot);
	}
#endif

	std::unordered_map<VirtualRegisterName*, PhysicalRegisterName*> map;

	for (const Interval& interval : intervals_sorted)
	{
		map.insert({ interval.virtual_register, interval.physical_register });
	}

	for (BasicBlock& bb : function->blocks()) {
		for (Instruction& insn : bb) {
			for (size_t op = 0; op < insn.GetCountOperands(); ++op) {
				if (VirtualRegisterName* vreg = value_cast<VirtualRegisterName>(insn.GetOperand(op))) {
					auto it = map.find(vreg);
					helix_assert(it != map.end(), "register not allocated for spill :(");
					insn.SetOperand(op, it->second);
				}
			}
		}
	}
}

/*********************************************************************************************************************/
