# Barney Wilks (18.1.2022)
# Liveness Analysis Experiments

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

    """ Set of variables whose values may be used in this
        block prior to any definitions of those variables. """
    def calc_uses(self):
        uses = set()

        for op in self.ops:
            for var in op.uses:
                uses.add(var)

        return uses

    """ Set of variables which are definately assigned values in
        in block prior to any use of those variables (in this block). """
    def calc_defs(self):
        defs = set()

        for op in self.ops:
            for var in op.defs:
                defs.add(var)

        return defs

blocks = [
    BasicBlock(), # 0, ENTRY (empty)
    BasicBlock(), # 1
    BasicBlock(), # 2
    BasicBlock(), # 3
    BasicBlock(), # 4
    BasicBlock()  # 5, EXIT (empty)
]

blocks[1].successors = {blocks[2], blocks[3]}
blocks[2].successors = {blocks[4]}
blocks[3].successors = {blocks[4]}
blocks[4].successors = {blocks[5]}

blocks[1].add_op(Operation(['b'], [])) # define 'b'
blocks[1].add_op(Operation(['a'], [])) # define 'a'
blocks[1].add_op(Operation(['c'], [])) # define 'c'
blocks[1].add_op(Operation(['d'], [])) # define 'd'
blocks[1].add_op(Operation([], ['d'])) # use 'd'

blocks[2].add_op(Operation([], ['b'])) # use 'b'

blocks[2].add_op(Operation([], ['c'])) # use 'c'

blocks[3].add_op(Operation([], ['d'])) # use 'd'

blocks[4].add_op(Operation([], ['b'])) # use 'b'
blocks[4].add_op(Operation([], ['a'])) # use 'a'

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
    print("\tIN  = " + str(live_in[blocks[i]]))
    print("\tOUT = " + str(live_out[blocks[i]]))
    print()

