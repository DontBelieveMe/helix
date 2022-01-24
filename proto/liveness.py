# Barney Wilks (18.1.2022)
# Liveness Analysis Experiments

import string
from functools import cmp_to_key


def print_intervals_table_header(cell_width):
    print(" " * cell_width, end = '')

    seperator_length = 0

    for block_index in range(0, len(blocks) - 1):
        block = blocks[block_index]

        for op_index in range(0, len(block.ops)):
            op = block.ops[op_index]

            if op_index == 0:
                print(str(block_index) + (' ' * (cell_width - 1)), end = '')
            else:
                print(' ' * cell_width, end = '')

            seperator_length += cell_width

            if op_index < len(block.ops) - 1:
                seperator_length += 1
                print(' ', end = '')

        print(' | ', end = '')
        seperator_length += 3

    print()
    print("  "  + ("-" * seperator_length))

def print_intervals_table_body(intervals, get_live_char, get_dead_char):
    for interval in intervals:
        print(interval.var + ':', end = '')

        for bi in range(0, len(blocks) - 1):
            block = blocks[bi]

            for oi in range(0, len(block.ops)):
                LIVE_CHAR = get_live_char(interval)
                DEAD_CHAR = get_dead_char(interval)

                c = DEAD_CHAR

                if bi == interval.start.block_index and oi >= interval.start.operation_index:
                    c = LIVE_CHAR
                elif bi == interval.end.block_index and oi <= interval.end.operation_index:
                    c = LIVE_CHAR
                elif bi > interval.start.block_index and bi < interval.end.block_index:
                    c = LIVE_CHAR

                print(c, end = '')

                if oi < len(block.ops) - 1:
                    print (' ', end = '')

            print(' | ', end = '')

        print('')

def print_intervals_table(cell_width, intervals, get_live_char, get_dead_char):
    print_intervals_table_header(cell_width)
    print_intervals_table_body(intervals, get_live_char, get_dead_char)

class Operation:
    def __init__(self, defs, uses):
        self.defs = defs
        self.uses = uses

class BasicBlock:
    def __init__(self):
        self.successors = set() 
        self.ops = []

    def add_op(self, op):
        self.ops.append(op)

    def print_ops(self, prefix):
        for op in self.ops:
            s = ', '.join(op.defs)
            s += " = "
            s += ', '.join(op.uses)
            print(prefix + s)

    """ Set of variables whose values may be used in this
        block prior to any definitions of those variables. """
    def calc_uses(self):
        uses         = set()
        defined_vars = set()

        for op in self.ops:
            for var in op.defs:
                defined_vars.add(var)

            for var in op.uses:
                # Only want variables that have not already
                # been defined in this block.
                if not var in defined_vars:
                    uses.add(var)

        return uses

    """ Set of variables which are definately assigned values in
        in block prior to any use of those variables (in this block). """
    def calc_defs(self):
        defs      = set()
        used_vars = set()

        for op in self.ops:
            for var in op.uses:
                used_vars.add(var)

            for var in op.defs:
                # Only want definitions that have not already
                # been used in this block.
                if not var in used_vars:
                    defs.add(var)

        return defs

blocks = [
    BasicBlock(), # 0, ENTRY (empty)
    BasicBlock(), # 1
    BasicBlock(), # 2
    BasicBlock(), # 3
    BasicBlock(), # 4,
    BasicBlock(), # 5
    BasicBlock(), # 6
    BasicBlock()  # 7, EXIT (empty)
]

#blocks[0].successors = {blocks[1]} # ENTRY
#blocks[6].successors = {blocks[7]} # EXIT

blocks[1].successors = {blocks[2]}
blocks[2].successors = {blocks[3]}
blocks[3].successors = {blocks[4],blocks[5]}
blocks[4].successors = {blocks[6]}
blocks[5].successors = {blocks[6]}

# e = d + a
blocks[1].add_op(Operation([], ['h']))
blocks[1].add_op(Operation(['e'], ['d', 'a']))
blocks[1].add_op(Operation(['j'], ['a', 'a']))
blocks[1].add_op(Operation(['z'], ['a']))

blocks[2].add_op(Operation([], ['z', 'e']))
blocks[2].add_op(Operation(['f'], ['b', 'c']))

blocks[3].add_op(Operation(['f'], ['f', 'b']))

blocks[4].add_op(Operation(['d'], ['e', 'f']))

blocks[5].add_op(Operation(['d'], ['e', 'f']))

blocks[6].add_op(Operation(['g'], ['d']))
blocks[6].add_op(Operation([], ['g']))
blocks[6].add_op(Operation([], ['j']))

# blocks[1].add_op(Operation(['b'], [])) # define 'b'
# blocks[1].add_op(Operation(['a'], [])) # define 'a'
# blocks[1].add_op(Operation(['c'], [])) # define 'c'
# blocks[1].add_op(Operation(['d'], [])) # define 'd'
# blocks[1].add_op(Operation([], ['d'])) # use 'd'
# 
# blocks[2].add_op(Operation([], ['b'])) # use 'b'
# blocks[2].add_op(Operation([], ['c'])) # use 'c'
# blocks[2].add_op(Operation([], ['a'])) # use 'a'
# 
# blocks[3].add_op(Operation([], ['d'])) # use 'd'
# blocks[3].add_op(Operation([], ['a'])) # use 'a'
# 
# blocks[4].add_op(Operation([], ['b'])) # use 'b'
# blocks[4].add_op(Operation([], ['a'])) # use 'a'
# blocks[4].add_op(Operation(['e'], [])) # define 'e'
# 
# blocks[5].add_op(Operation([], ['e'])) # use 'e'

def annotate_block_index(index):
    if index == 0:
        return " [ENTRY (empty)]"
    elif index == len(blocks) - 1:
        return " [EXIT (empty)]"

    return ""

for i in range(0, len(blocks)):
    s = annotate_block_index(i)
    print("Basic Block " + str(i) + s + ":")
    print("\tUses = " + str(blocks[i].calc_uses()))
    print("\tDefs = " + str(blocks[i].calc_defs()))

    print()

print("Mayday, Mayday, Mayday, Liveness Analysis Inbound. Brace, Brace, Brace!")
print()

# Liveness Analysis Algorithm

# IN[?]
#   key   = basic block,
#   value = set of variables live on entry to that block
live_in  = {}

# OUT[?]
#   key = basic block
#   value = set of variables live on exit of that block
live_out = {}

# Compute the (current) set of variables live on entry to the given
# block.
# Not standalone, is part of an iterative algorithm
def compute_live_in(block):
    uses = block.calc_uses()
    defs = block.calc_defs()

    return uses.union(live_out[block].difference(defs))

# Compute the (current) set of variables live on exit of the given
# block.
# Not standalone, is part of an iterative algorithm
def compute_live_out(block):
    block_live_out = set()

    for successor in block.successors:
        block_live_out = block_live_out.union(live_in[successor])

    return block_live_out

# Assumption that last block in list is the terminating
# block (kindof makes sense? can be assumed in hxc, after
# the return combine pass has run)
exit_block = blocks[len(blocks) - 1]

# (1) IN[EXIT] = {}
live_in[exit_block] = set()

# (2) for (each basic block B other than exit) IN[B] = {};
for bb in blocks:
    live_out[bb] = set()

    if bb != exit_block: # 'other than exit'
        live_in[bb] = set()

# (3) while (changes to IN occur)
count_iterations = 0

while True:
    dirty = False

    # (4) for (each basic block B other than exit)
    for bb in blocks:
        if bb != exit_block: # 'other than exit'

            # (5) Compute OUT[bb]
            live_out[bb] = compute_live_out(bb)

            # (6) Compute IN[bb]
            temp = live_in[bb]
            live_in[bb] = compute_live_in(bb)

            # (7) If IN[bb] is the same as before running compute_live_in
            #     it means it didn't change, so we don't need to recompute
            #     live out for this block. Otherwise if IN[bb] *did* change
            #     then we'll need to run another iteration to recompute OUT[bb].
            #     (that in turn could cause IN[bb] to change again, oh the humanity!)
            if temp != live_in[bb]:
                dirty = True
    
    count_iterations += 1

    # If we didn't make any changes to IN for any block
    # then the algorithm has finished.
    # Otherwise we need to rerun to recompute OUT for some blocks
    if not dirty:
        break

# Output Results

print("Computed in " + str(count_iterations) + " iterations")
print()

for i in range(0, len(blocks)):
    s = annotate_block_index(i)
    print("Basic Block " + str(i) + s + ":")
    blocks[i].print_ops('\t> ')
    print()
    print("\tIN  = " + str(sorted(live_in[blocks[i]])))
    print("\tOUT = " + str(sorted(live_out[blocks[i]])))
    print()


# Live Interval Analysis

# Algorithm:
#   http://web.cs.ucla.edu/~palsberg/course/cs132/linearscan.pdf
#
# Paper is vauge on specifcs of how live interval analysis is done, just
# that given live variable information, it can be computed "easily" in one
# pass... Well i'll show them how hard i can make it!
#
# INPUT:
#   LIVE_IN  -> Variables that are live on entry to each basic block
#   LIVE_OUT -> Variables that are live on exit of each basic block
#
# OUTPUT:
#   Interval[] -> Array of intervals for each variable
#
# ALGORITHM:
#
#  

class OpIndex:
    def __init__(self, block_index, operation_index):
        self.block_index     = block_index
        self.operation_index = operation_index

def stringify_opindex(opindex):
    if opindex == None:
        return "?"

    return str(opindex.block_index) + ":" + str(opindex.operation_index)

class Interval:
    def __init__(self, var, start, end):
        self.var   = var
        self.start = start
        self.end   = end
        self.reg   = '??'
        self.loc   = 'xx'

intervals = {}

for block_index in range(0, len(blocks)):
    block = blocks[block_index]

    uses = {}

    for i in range(0, len(block.ops)):
        insn = block.ops[i]

        for d in insn.uses:
            if not d in live_in[block] and not d in live_out[block]:
                uses[d] = Interval(d, None, OpIndex(block_index, i))

    for i in range(0, len(block.ops)):
        insn = block.ops[i]

        for d in insn.defs:
            if d in uses:
                if not d in live_in[block] and not d in live_out[block]:
                    uses[d].start = OpIndex(block_index, i)
                    intervals[d] = uses[d]

    # Handle live in variables
    for variable in live_in[block]:
        if not variable in intervals:
            start = OpIndex(block_index, 0)
            end = None

            intervals[variable] = Interval(variable, start, end)

    for variable in live_in[block]:
        if not variable in live_out[block]:
            end = OpIndex(block_index, -1)

            for i in range(0, len(block.ops)):
                insn = block.ops[i]

                if variable in insn.uses:
                    end.operation_index = i

            intervals[variable].end = end

    for variable in live_out[block]:
        if not variable in live_in[block] and not variable in intervals:
            start = OpIndex(block_index, -1)

            for i in range(0, len(block.ops)):
                insn = block.ops[i]

                if variable in insn.defs:
                    start.operation_index = i

            end = None

            intervals[variable] = Interval(variable, start, end)
        else:
            end = OpIndex(block_index, -1)

            for i in range(0, len(block.ops)):
                insn = block.ops[i]

                if variable in insn.uses:
                    end.operation_index = i

            intervals[variable].end = end

for bi in range(0, len(blocks)):
    block = blocks[bi]

    block.print_ops("(" + str(bi) + ") ")

print()

for variable in intervals:
    interval = intervals[variable]
    print(variable + " = " + stringify_opindex(interval.start) + " -> " + stringify_opindex(interval.end))

print()

print_intervals_table(
    1,
    [intervals[v] for v in intervals],
    lambda _: 'x',
    lambda _: '-'
)

print()    

########################################################
# Linear Scan Register Allocation
########################################################
#
# http://web.cs.ucla.edu/~palsberg/course/cs132/linearscan.pdf
#
# 1) Sort intervals by increasing start point

print()
print("============= Let the games begin! =============")

def compare_interval_start(a, b):
    if a.start.block_index < b.start.block_index:
        return -1
    elif a.start.block_index > b.start.block_index:
        return 1
    else:
        if a.start.operation_index < b.start.operation_index:
            return -1
        elif a.start.operation_index > b.start.operation_index:
            return 1
        else:
            return 0

def compare_interval_end(a, b):
    if a.end.block_index < b.end.block_index:
        return -1
    elif a.end.block_index > b.end.block_index:
        return 1
    else:
        if a.end.operation_index < b.end.operation_index:
            return -1
        elif a.end.operation_index > b.end.operation_index:
            return 1
        else:
            return 0

def compare_interval_end_start(a, b):
    if a.end.block_index < b.start.block_index:
        return -1
    elif a.end.block_index > b.start.block_index:
        return 1
    else:
        if a.end.operation_index < b.start.operation_index:
            return -1
        elif a.end.operation_index > b.start.operation_index:
            return 1
        else:
            return 0


intervals_sorted = [intervals[v] for v in intervals]
intervals_sorted.sort(key = cmp_to_key(compare_interval_start))

# 2) Let's go!

# List of active intervals, normally sorted by endpoint
active = []

# Used to allocate new stack location
stack_counter = 0

# Set of all currently free registers
free_registers = {'r0', 'r1', 'r2', 'r3', 'r4', 'r5', 'r6', 'r7'}

# Total number of registers (that coulld be allocated at once)
# Note that the length of free_registers will change as different
# registers get allocated to active intervals, but this should be
# constant (defined in terms of the length of free_registers for conveniance
# in practice this is fixed)
COUNT_REGS = len(free_registers)

# Allocate a new stack location for a spill
def new_stack_location():
    global stack_counter
    s = "s" + str(stack_counter) + ""
    stack_counter += 1
    return s

def spill_at_interval(interval):
    spill = active[len(active) - 1]

    # Take the furthest away interval and see if it ends after this
    # given interval.
    # In that case it is preferable to spill that interval to the
    # stack and use that register for the given interval.
    if compare_interval_end(spill, interval) > 0:
        interval.reg = spill.reg
        spill.loc    = new_stack_location()

        # Remove the variable we've just spilled from the active list
        # and add the given interval...
        active.remove(spill)
        active.append(interval)

        # ... ensuring that the active list is still sorted by
        # increasing end point
        active.sort(key = cmp_to_key(compare_interval_end))
    else:
        # Otherwise just allocate a new stack location for this variable
        interval.loc = new_stack_location()

def expire_old_intervals(interval):
    global active

    # List of intervals that are still active once we've
    # found any that need to be expired
    keep = []

    for active_interval in active:
        comparison = compare_interval_end_start(active_interval, interval)

        # If this (currently active) interval ends after the current interval
        # then it is still active (so keep it), otherwise it needs to be
        # expired
        if comparison >= 0:
            keep.append(active_interval)

            # STUPID BUG ALERT!
            # 
            # Code in Paper is as follows for this section:
            #
            #     > if endpoint[j] ≥ startpoint[i] then
            #     >    return
            #
            # I previously copied this verbaitam but it
            # it really makes sense for this to be continue
            # (had bugs around spilling when it wasn't nessesary due
            # to intervals not being removed from the active list when
            # expired. changing 'return' to 'continue' fixed this.)
            # FIXME: Investigate this!
            continue
        
        # This interval has expired so add its register back to the free list
        free_registers.add(active_interval.reg)

    active = keep

    # Ensure the active interval list is sorted by increasing endpoint
    active.sort(key = cmp_to_key(compare_interval_end))

for interval in intervals_sorted:
    expire_old_intervals(interval)

    # Worst case senario \/
    if len(active) >= COUNT_REGS:
        spill_at_interval(interval)
    else:
        interval.reg = free_registers.pop()
        
        active.append(interval)
        active.sort(key = cmp_to_key(compare_interval_end))

# ========== Debug Dump ==========

print()

# Print each interval & it's assigned register/stack location
for interval in intervals_sorted:
    print("{} = {} -> {} ({}, {})".format(
        interval.var,
        stringify_opindex(interval.start),
        stringify_opindex(interval.end),
        interval.reg,
        interval.loc
    ))

print()

def get_interval_live_char(interval):
    if interval.reg == '??':
        return interval.loc

    return interval.reg

# Print a table showing each interval & it's assigned register
# over it's range
print_intervals_table(
    2,
    intervals_sorted,
    get_interval_live_char,
    lambda _: '--'
)

print()
