/**
 * @file ir-helpers.cpp
 * @author Barney Wilks
 *
 * Implementation of ir-helpers.h
 */

/* Internal Project Includes */
#include "ir-helpers.h"

/* Standard Library Includes */
#include <vector>

using namespace Helix;

/*********************************************************************************************************************/

void IR::ReplaceAllUsesWith(Value* oldValue, Value* newValue)
{
	std::vector<Use> worklist;

	for (auto it = oldValue->uses_begin(); it != oldValue->uses_end(); ++it) {
		worklist.push_back(*it);
	}

	for (Use& use : worklist) {
		use.ReplaceWith(newValue);
	}
}

/*********************************************************************************************************************/
