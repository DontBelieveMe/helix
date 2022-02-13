/**
 * @file stack-frame.cpp
 * @author Barney Wilks
 *
 * Implements stack-frame.h
 */

/* Internal Project Includes */
#include "stack-frame.h"
#include "system.h"

using namespace Helix;

/*********************************************************************************************************************/

static unsigned Align(unsigned input, unsigned alignment)
{
	if (input % alignment == 0) {
		return input;
	}

	return input + (alignment - (input % alignment));
}

/*********************************************************************************************************************/

StackFrame::SlotIndex StackFrame::Add(size_t bytes)
{
	const SlotIndex slot { m_Allocations.size() };

	m_Allocations.push_back({ bytes, m_NextAllocationOffset });

	m_NextAllocationOffset += bytes;

	return slot;
}

/*********************************************************************************************************************/

size_t StackFrame::GetAllocationOffset(SlotIndex slotIndex) const
{
	helix_assert(slotIndex.index < m_Allocations.size(), "StackSlot index is too big");
	return m_Allocations[slotIndex.index].Offset;
}

/*********************************************************************************************************************/

size_t StackFrame::GetAllocationSize(SlotIndex slotIndex) const
{
	helix_assert(slotIndex.index < m_Allocations.size(), "StackSlot index is too big");
	return m_Allocations[slotIndex.index].Size;
}

/*********************************************************************************************************************/

size_t StackFrame::GetSizeAligned(size_t alignment) const
{
	return Align(m_NextAllocationOffset, alignment);
}

/*********************************************************************************************************************/