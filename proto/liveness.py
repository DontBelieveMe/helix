# Barney Wilks (18.1.2022)
# Liveness Analysis Experiments

import string


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

blocks[2].add_op(Operation(['f'], ['b', 'c']))

blocks[3].add_op(Operation(['f'], ['f', 'b']))

blocks[4].add_op(Operation(['d'], ['e', 'f']))

blocks[5].add_op(Operation(['d'], ['e', 'f']))

blocks[6].add_op(Operation(['g'], ['d']))
blocks[6].add_op(Operation([], ['g']))

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
print("  ", end='')

l=0

for bi in range(0, len(blocks) - 1):
    block = blocks[bi]

    for oi in range(0, len(block.ops)):
        op = block.ops[oi]

        if oi == 0:
            print(str(bi), end='')
        else:
            print(' ', end='')

        l += 1

        if oi < len(block.ops) - 1:
            l += 1
            print(' ', end='')

    print(" | ", end='')

    l += 3

print("")

print("  "  + ("-" * l))

for variable in intervals:
    interval = intervals[variable]

    print(variable + ":", end='')

    for bi in range(0, len(blocks) - 1):
        block = blocks[bi]

        for oi in range(0, len(block.ops)):
            op = block.ops[oi]

            LIVE_CHAR = 'x'
            DEAD_CHAR = '-'

            c = DEAD_CHAR

            if bi == interval.start.block_index and oi >= interval.start.operation_index:
                c = LIVE_CHAR
            elif bi == interval.end.block_index and oi <= interval.end.operation_index:
                c = LIVE_CHAR
            elif bi > interval.start.block_index and bi < interval.end.block_index:
                c = LIVE_CHAR

            print(c, end='')

            if oi < len(block.ops) - 1:
                print (' ', end='')

        print(" | ", end='')

    print("")

print()    


