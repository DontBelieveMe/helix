/**
 * @file stack-frame.h
 * @author Barney Wilks
 */

#pragma once

/* C++ Standard Library Includes */
#include <vector>

namespace Helix
{
	class StackFrame
	{
	public:
		struct SlotIndex
		{
			size_t index = SIZE_MAX;
		
			bool IsValid() const { return index != SIZE_MAX; }
		};

		/**
		 * Add a new element to the stack frame, returning the slot index.
		 * 
		 * @param bytes The size (in bytes) of the item/allocation
		 *
		 * @return The stack slot representing this item.
		 */
		SlotIndex Add(size_t bytes);

		size_t GetAllocationOffset(SlotIndex slotIndex) const;
		size_t GetAllocationSize(SlotIndex slotIndex) const;

		/**
		 * Get the size of the stack (in bytes), aligned to the given
		 * alignment (GetSizeAligned() MOD alignment = 0)
		 */
		size_t GetSizeAligned(size_t alignment) const;

	private:
		struct Allocation
		{
			/// Size of this stack allocation
			size_t Size;

			/// Offset in bytes from the start of the stack
			size_t Offset;
		};

		const Allocation* GetAllocationPtr(SlotIndex index) const;
		bool IsValidSlot(SlotIndex index) const;

		/// List of all individual elements in this stack frame
		std::vector<Allocation> m_Allocations;

		/// An offset (in bytes) from the start of the stack frame, where
		/// we will put the next allocation
		size_t m_NextAllocationOffset = 0;
	};
}
