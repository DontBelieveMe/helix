/**
 * @file linear-scan.h
 * @author Barney Wilks
 *
 * Implements linear scan register allocation
 */

#pragma once

/* Internal Project Includes */
#include "interval.h"

/* C++ Standard Library Includes */
#include <unordered_map>

namespace Helix
{
	class VirtualRegisterName;
	class PhysicalRegisterName;

	/// Linear Scan Register Allocation
	namespace LSRA
	{
		struct Context
		{
			std::unordered_map<VirtualRegisterName*, Interval> InputIntervals;
			std::unordered_map<VirtualRegisterName*, PhysicalRegisterName*> Allocated;
		};

		void Run(Context* context);
	}
}