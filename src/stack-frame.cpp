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

/******************************************************************************/

static size_t
Align(size_t input, size_t alignment)
{
	if (input % alignment == 0) {
		return input;
	}

	return input + (alignment - (input % alignment));
}

/******************************************************************************/

bool
StackFrame::IsValidSlot(SlotIndex index) const
{
	return index.IsValid() && (index.index < m_Allocations.size());
}

/******************************************************************************/

const StackFrame::Allocation*
StackFrame::GetAllocationPtr(SlotIndex index) const
{
	helix_assert(IsValidSlot(index), "StackSlot index is too big");

	return &m_Allocations[index.index];
}

/******************************************************************************/

StackFrame::SlotIndex
StackFrame::Add(size_t bytes)
{
	const SlotIndex slot { m_Allocations.size() };

	m_NextAllocationOffset += bytes;
	m_Allocations.push_back({ bytes, m_NextAllocationOffset });

	return slot;
}

/******************************************************************************/

size_t
StackFrame::GetAllocationOffset(SlotIndex slotIndex) const
{
	return GetAllocationPtr(slotIndex)->Offset;
}

/******************************************************************************/

size_t
StackFrame::GetAllocationSize(SlotIndex slotIndex) const
{
	return GetAllocationPtr(slotIndex)->Size;
}

/******************************************************************************/

size_t
StackFrame::GetSizeAligned(size_t alignment) const
{
	return Align(m_NextAllocationOffset, alignment);
}

/******************************************************************************/
