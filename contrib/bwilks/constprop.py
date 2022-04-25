from tkinter import Variable
from tokenize import Number
import graphviz

STMT_ASSIGN = 0
STMT_OTHER  = 1
STMT_BINOP  = 2

OpcodeStringMap = {
    STMT_ASSIGN: "=",
    STMT_OTHER: "?(...)",
    STMT_BINOP: "+"
}

class Stmt:
    def __init__(self, opcode, reads, writes):
        self.opcode = opcode
        self.reads = reads
        self.writes = writes
        self.parent = None

    def get_assignment_rhs(self):
        return self.reads[0]

    def __str__(self):
        r = ""

        for r2 in self.reads:
            r += str(r2) + ","

        w = ""

        for w2 in self.writes:
            w += str(w2) + ","

        return "{}: reads({}), writes({})".format(self.opcode, r, w)

class BasicBlock:
    def __init__(self, stmts, preds):
        self.stmts = stmts
        self.preds = preds

        for stmt in self.stmts:
            stmt.parent = self

def DumpBlocks(blocks):
    for i in range(0, len(blocks)):
        print(".block " + str(i))

        print("\t<preds ", end='')

        for pred in blocks[i].preds:
            print(str(pred) + ",", end='')

        print(">\n")

        for stmt in blocks[i].stmts:
            print("\t", end='')

            for arg in stmt.writes:
                print(arg, end=', ')

            print("<- " + OpcodeStringMap[stmt.opcode] + " ", end='')

            for arg in stmt.reads:
                print(arg, end=', ')

            print("")
        print()

def DisplayGraphBlocks(blocks):
    dot = graphviz.Digraph('CFG', comment='Control Flow Graph')

    for i in range(0, len(blocks)):
        block = blocks[i]

        s = ''

        for stmt in block.stmts:
            for arg in stmt.writes:
                s += str(arg) + ', '

            s += OpcodeStringMap[stmt.opcode] + " "

            for arg in stmt.reads:
                s += str(arg) + ", "

            s += "\n"

        dot.node(str(i), "Block " + str(i) + "\n\n" + s)
        
        for pred in block.preds:
            dot.edge(str(pred), str(i))

    dot.render(directory='graphiz')


blocks = [
    #                OPCODE       READS     WRITES   PREDS (BLOCK INDICES)
    BasicBlock([
        Stmt(STMT_ASSIGN, [1],      ['a']),
        Stmt(STMT_ASSIGN, [4],      ['d']),
        Stmt(STMT_ASSIGN, [543],      ['f']),
        Stmt(STMT_ASSIGN, [500],      ['e']),
        Stmt(STMT_ASSIGN, [2],      ['b']),
        ], [    ]),
    BasicBlock([
        Stmt(STMT_ASSIGN, [2],      ['b']),
        Stmt(STMT_ASSIGN, [98],      ['e'])
    ], [0   ]),
    BasicBlock([
        Stmt(STMT_BINOP,  ['a','b'],['c']),
        Stmt(STMT_OTHER,  ['f'],[]),
        Stmt(STMT_OTHER,  ['e'],[]),
    ], [0, 1]),
    BasicBlock([
        Stmt(STMT_BINOP,  ['c', 4],['c']),
        Stmt(STMT_OTHER,  ['c'],[]),
        Stmt(STMT_OTHER,  ['f'],[])
    ], [2, 6]),
    BasicBlock([Stmt(STMT_ASSIGN,  [50],['f'])], [3]),
    BasicBlock([Stmt(STMT_ASSIGN,  [40],['f'])], [3]),
    BasicBlock([
        Stmt(STMT_OTHER,  ['f'],[]),
        Stmt(STMT_ASSIGN, [543],      ['f']),
        Stmt(STMT_OTHER,  ['f'],[])
    ], [4,5])
]

DumpBlocks(blocks)

# DisplayGraphBlocks(blocks)

# Constant Propagation Algorithm

VALUE_CONSTANT = 0
VALUE_TOP      = 1
VALUE_BOTTOM   = 2

class Value:
    def __init__(self, constant, typ):
        self.constant = constant
        self.type     = typ

    def __eq__(self, other):
        return self.constant == other.constant and self.type == other.type

    def __str__(self):
        if self.type == VALUE_CONSTANT:
            return str(self.constant)

        if self.type == VALUE_BOTTOM:
            return "⊥"

        if self.type == VALUE_TOP:
            return "⊤"

    def __hash__(self):
        return hash((self.constant, self.type))

def GetValue(constant):
    if type(constant) is Value:
        if constant == TOP:
            return TOP

        if constant == BOTTOM:
            return BOTTOM

    return Value(constant, VALUE_CONSTANT)

TOP = Value(0, VALUE_TOP)
BOTTOM = Value(0, VALUE_BOTTOM)

def Meet(v0, v1):
    if v1 == TOP:
        return v0
    
    if v1 == BOTTOM:
        return BOTTOM

    if v0 == v1:
        return v1
    else:
        return BOTTOM

def MeetFromSet(vs):
    if len(vs) == 0:
        return BOTTOM

    vs = list(vs)

    if len(vs) == 1:
        return vs[0]

    v = Meet(vs[0], vs[1])

    for i in range(2, len(vs)):
        v = Meet(v, vs[i])

    return v

print("TOP = variable may be some (as yet) undetermined constant")
print("BOTTOM = constant value cannot be guaranteed")

print(Meet(GetValue(0), TOP))
print(Meet(GetValue(4), BOTTOM))
print(Meet(TOP, BOTTOM))
print(Meet(BOTTOM, BOTTOM))
print(Meet(BOTTOM, TOP))
print(Meet(TOP, TOP))
print(Meet(GetValue(5), GetValue(5)))
print(Meet(GetValue(0), GetValue(2)))
print()

print("----------------------------")
print(MeetFromSet([  GetValue(10), GetValue(10), GetValue(10) ]))
print(MeetFromSet([  GetValue(10), GetValue(20), GetValue(10) ]))
print(MeetFromSet([  GetValue(TOP), GetValue(TOP), GetValue(TOP) ]))
print(MeetFromSet([  GetValue(TOP), GetValue(TOP), GetValue(BOTTOM) ]))
print(MeetFromSet([ BOTTOM, BOTTOM, BOTTOM ]))
print(MeetFromSet([ BOTTOM, BOTTOM, TOP ]))
print(MeetFromSet([GetValue(10), GetValue(10), TOP]))
print(MeetFromSet([GetValue(10), GetValue(10), BOTTOM]))
print(MeetFromSet([ BOTTOM, TOP ]))
print(MeetFromSet([ TOP, BOTTOM ]))


print("----------------------------")

class VariableMap:
    def __init__(self):
        self.map = {}

    def get(self, v):
        if v in self.map:
            return self.map[v]

        self.map[v] = TOP
        return TOP

    def set(self, v, value):
        self.map[v] = value

    def __str__(self):
        s = ''

        for key in self.map:
            s += '"' + str(key) + '": ' + str(self.map[key]) + ', '

        return s

    def clone(self):
        v = VariableMap()
        v.map = self.map.copy()
        return v

    def __eq__(self, other):
        return self.map == other.map

class Node:
    def __init__(self, block, stmt):
        self.preds = set()
        self.stmt = stmt
        self.block = block

        self.cin = VariableMap()
        self.cout = VariableMap()
        self.ni = -1

    def __str__(self):
        p = ""
        for ps in self.preds:
            p += str(ps) + ","

        return "preds({})\t[{}]\t IN {}, || OUT {}".format(p, self.stmt, self.cin, self.cout)

def DumpNodes(nodes):
    for i in range(0, len(nodes)):
        print(str(hex(id(nodes[i]))) + " -> " + str(i) + ") " + str(nodes[i]))

block_start_map = {}
block_end_map = {}

nodes = []

all_vars = set()

ni = 0
for block in blocks:
    block_start_map[block] = ni

    for stmt in block.stmts:
        node = Node(block, stmt)
        node.ni = ni
        nodes.append(node)
        ni = ni + 1
        preds = None

        for r in stmt.reads:
            if type(r) is str:
                all_vars.add(r)

        for w in stmt.writes:
            if type(w) is str:
                all_vars.add(w)

    block_end_map[block] = ni

for block in block_start_map:
    index = block_start_map[block]

    preds = set()

    for p in block.preds:
        preds.add(block_end_map[blocks[p]] - 1)

    nodes[index].preds = preds

for i in range(1, len(nodes)):
    nodes[i].preds.add(i - 1)

for var in all_vars:
    nodes[0].cin.set(var, BOTTOM)


print("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@")
DumpNodes(nodes)
print("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@")

# The lattice value stored at the
# entry to the examined node becomes the meet of values at the exits of
# preceding nodes

worklist = []

for node in nodes:
    worklist.append(node)

def ComputeIn(node):
    out = node.cin.clone()

    all_variables = {}

    for pred_index in node.preds:
        pred_node = nodes[pred_index]

        for variable in pred_node.cout.map:
            if variable in all_variables:
                all_variables[variable].append(pred_node.cout.get(variable))
            else:
                all_variables[variable] = [ pred_node.cout.get(variable) ]

    for var in all_variables:
        out.set(var, MeetFromSet(all_variables[var]))

    return out

def ComputeOut(node):

    for var in node.cin.map:
        node.cout.set(var, node.cin.get(var))

    for var in node.stmt.writes:
        if node.stmt.opcode == STMT_ASSIGN:
            node.cout.set(var, GetValue(node.stmt.get_assignment_rhs()))
        else:
            if node.stmt.opcode == STMT_BINOP:
                lhs = node.stmt.reads[0]
                rhs = node.stmt.reads[1]

                if type(lhs) is int and type(rhs) is int:
                    node.cout.set(var, GetValue(lhs + rhs))
                else:
                    if type(lhs) is str:
                        lhs = node.cin.get(lhs)

                    if type(rhs) is str:
                        rhs = node.cin.get(rhs)

                    if type(lhs) is Value and lhs.type == VALUE_CONSTANT:
                        lhs = lhs.constant

                    if type(rhs) is Value and rhs.type == VALUE_CONSTANT:
                        rhs = rhs.constant

                    if type(lhs) is int and type(rhs) is int:
                        node.cout.set(var, GetValue(lhs + rhs))
                        return
                    
                    node.cout.set(var, BOTTOM)
            else:
                node.cout.set(var, BOTTOM)

while len(worklist) > 0:
    n = worklist.pop(0)
    tmp = ComputeIn(n)
    print(">> " + str(n.ni) + " : " + str(n))

    if tmp != n.cin:
        n.cin = tmp

        for p in n.preds:
            worklist.append(nodes[p])

    ComputeOut(n)

print("*****************************************************")
DumpNodes(nodes)
print("*****************************************************")

def DumpNodes2(nodes):
    bi = 0
    prev_block = None
    for i in range(0, len(nodes)):

        if nodes[i].block != prev_block:
            prev_block = nodes[i].block
            print(".block " + str(bi))
            bi += 1

        print("\t{}: [{}]".format(i, str(nodes[i].stmt)))
        
        if len(nodes[i].cin.map) > 0:
            print("\t\tIN:",end=' ')
            for var in nodes[i].cin.map:
                print("\t" + str(var) + " = " + str(nodes[i].cin.get(var)), end=',')
            print()

        if len(nodes[i].cout.map) > 0:
            print("\t\tOUT:",end=' ')
            for var in nodes[i].cout.map:
                print("\t" + str(var) + " = " + str(nodes[i].cout.get(var)), end=',')
            print()

def Rewrite(nodes):
    for node in nodes:
        s = node.stmt

        if s.opcode == STMT_BINOP:
            if node.cout.map[s.writes[0]].type == VALUE_CONSTANT:
                s.opcode = STMT_ASSIGN
                s.reads = [ node.cout.map[s.writes[0]] ]
                continue

        for i in range(0, len(node.stmt.reads)):
            if node.stmt.reads[i] in node.cin.map:
                v = node.cin.get(node.stmt.reads[i])

                if v.type == VALUE_CONSTANT:
                    node.stmt.reads[i] = v

#DisplayGraphBlocks(blocks)
Rewrite(nodes)
DumpNodes2(nodes)
DumpBlocks(blocks)
DisplayGraphBlocks(blocks)