/**
 * @file interval.cpp
 * @author Barney Wilks
 *
 * Implements interval.h
 */

/* Internal Project Includes */
#include "interval.h"
#include "instructions.h"
#include "function.h"
#include "ir-helpers.h"

using namespace Helix;

/*********************************************************************************************************************/

Interval::Interval(VirtualRegisterName* variable, InstructionIndex start, InstructionIndex end)
	: virtual_register(variable), start(start), end(end)
{ }

/*********************************************************************************************************************/

Interval::Interval(VirtualRegisterName* variable)
	: virtual_register(variable)
{ }

/*********************************************************************************************************************/

bool Interval::operator==(const Interval& other) const
{
	// Define equality in terms of the actual range of the interval
	// (and don't involve the extra 'fluff', e.g. the variable this represents
	// or the allocated register etc...)
	//
	// #FIXME(bwilks): Is doing this wise? Might cause some confusion...

	return start == other.start && end == other.end;
}

/*********************************************************************************************************************/

bool IntervalStartComparator::operator()(const Interval& a, const Interval& b) const
{
	return a.start < b.start;
}

/*********************************************************************************************************************/

bool IntervalEndComparator::operator()(const Interval& a, const Interval& b) const
{
	return a.end < b.end;
}

/*********************************************************************************************************************/

bool IntervalEndStartComparator::operator()(const Interval& a, const Interval& b) const
{
	return a.end < b.start;
}

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

void Helix::ComputeIntervalsForFunction(Function* function, std::unordered_map<VirtualRegisterName*, Interval>& intervals)
{
	// #FIXME(bwilks): Reference suggests that this can be done in one pass (which is _not_ the case here) have another
	//                 look & try and simplify it (probably could do with some tests first)

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

					// #FIXME(bwilks): This is a hack to get around intervals not being created for values
					//                 that are only defined & never used.
					if (IR::GetCountReadUsers(vreg) == 0) {
						const InstructionIndex here(blockIndex, instructionIndex);

						if (Contains(intervals, vreg)) {
							intervals[vreg].end = here;
						}
						else {
							intervals[vreg] = Interval(vreg, here, here);
						}

						continue;
					}

					if (!Contains(uses, vreg))
						continue;

					if (!Contains(block_in, vreg) && !Contains(block_out, vreg) && !Contains(intervals, vreg)) {
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
				if (Contains(block_in, vreg)) {
					intervals[vreg].end = InstructionIndex(blockIndex, bb.GetCountInstructions());
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
		}

		blockIndex++;
	}
}
