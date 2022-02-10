/**
 * @file interval.cpp
 * @author Barney Wilks
 *
 * Implements interval.h
 */

/* Internal Project Includes */
#include "interval.h"
#include "instructions.h"

using namespace Helix;

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

