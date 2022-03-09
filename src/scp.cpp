/**
 * @file scp.h
 * @author Barney Wilks
 *
 * Implementation of Kildall’s Simple Constant (SC) algorithm for constant
 * propagation
 *
 * (as described in Constant Propagation with Conditional Branches - MARK N. WEGMAN and F. KENNETH ZADECK)
 *
 * Implementation for interface defined in scp.h
 */

/* Internal Project Includes */
#include "scp.h"
#include "value.h"
#include "system.h"
#include "iterator-range.h"
#include "function.h"
#include "ir-helpers.h"
#include "print.h"

/* C++ Standard Library Includes */
#include <vector>
#include <unordered_map>

using namespace Helix;

/*********************************************************************************************************************/

class LatticeCell
{
public:
	enum Type
	{
		/// Variable is known to be this integral constant.
		kConstant,

		/// Variable may be some (as yet) undetermined constant.
		kTop,

		/// Constant value cannot be guaranteed.
		kBottom
	};

	static LatticeCell* GetTop()    { return &s_Top; }
	static LatticeCell* GetBottom() { return &s_Bottom; }

	static LatticeCell* GetValue(ConstantInt* constantInteger);

	ConstantInt* GetConstant() { return m_ConstantInteger; }

	bool IsTop()    const { return m_Type == kTop;    }
	bool IsBottom() const { return m_Type == kBottom; }
	bool IsConstant() const { return m_Type == kConstant;  }

private:
	LatticeCell(Type type)
		: m_Type(type), m_ConstantInteger(nullptr) { }

	LatticeCell(ConstantInt* c)
		: m_Type(kConstant), m_ConstantInteger(c) { }

private:

	ConstantInt*                                          m_ConstantInteger;
	Type                                                  m_Type;

	static LatticeCell                                    s_Top;
	static LatticeCell                                    s_Bottom;
	static std::unordered_map<ConstantInt*, LatticeCell*> s_ConstantCellCache;
};

/*********************************************************************************************************************/

class VariableMap
{
	using CellMapType = std::unordered_map<VirtualRegisterName*, LatticeCell*>;

public:
	LatticeCell* Get(VirtualRegisterName* variable);
	void         Set(VirtualRegisterName* variable, LatticeCell* cell);

	iterator_range<CellMapType::iterator> vars()
		{ return iterator_range(m_Cells.begin(), m_Cells.end()); }

	iterator_range<CellMapType::const_iterator> vars() const
		{ return iterator_range(m_Cells.begin(), m_Cells.end()); }

	bool operator==(const VariableMap& other) const
		{ return m_Cells == other.m_Cells; }

	bool operator!=(const VariableMap& other) const
		{ return !operator==(other); }

private:
	CellMapType m_Cells;
};

/*********************************************************************************************************************/

class Node
{
public:
	Node(Instruction* insn, size_t index)
		: m_Instruction(insn), m_Index(index) { }

	void AddPredecessor(size_t index);

	VariableMap* GetInputs();
	VariableMap* GetOutputs();

	const VariableMap* GetOutputs() const { return &m_Output; }
	const VariableMap* GetInputs() const { return &m_Input; }

	void SetInputs(const VariableMap& other)
		{ m_Input = other; }

	iterator_range<std::vector<size_t>::iterator> predecessors()
		{ return iterator_range(m_Predecessors.begin(), m_Predecessors.end()); }

	iterator_range<std::vector<size_t>::const_iterator> predecessors() const
		{ return iterator_range(m_Predecessors.begin(), m_Predecessors.end()); }

	Instruction* GetInsn() const { return m_Instruction; }

	size_t GetIndex() const { return m_Index;  }

private:
	size_t m_Index;
	Instruction* m_Instruction;

	std::vector<size_t> m_Predecessors;

	VariableMap m_Input;
	VariableMap m_Output;
};

/*********************************************************************************************************************/

void Node::AddPredecessor(size_t index)
{
	if (std::find(m_Predecessors.begin(), m_Predecessors.end(), index) == m_Predecessors.end())
		m_Predecessors.push_back(index);
}

/*********************************************************************************************************************/

VariableMap* Node::GetInputs()
{
	return &m_Input;
}

/*********************************************************************************************************************/

VariableMap* Node::GetOutputs()
{
	return &m_Output;
}

/*********************************************************************************************************************/

LatticeCell* VariableMap::Get(VirtualRegisterName* variable)
{
	auto it = m_Cells.find(variable);

	if (it == m_Cells.end()) {
		m_Cells[variable] = LatticeCell::GetTop();
		return LatticeCell::GetTop();
	}

	return it->second;
}

/*********************************************************************************************************************/

void VariableMap::Set(VirtualRegisterName* variable, LatticeCell* cell)
{
	m_Cells[variable] = cell;
}

/*********************************************************************************************************************/

LatticeCell LatticeCell::s_Top(LatticeCell::kTop);
LatticeCell LatticeCell::s_Bottom(LatticeCell::kBottom);
std::unordered_map<ConstantInt*, LatticeCell*> LatticeCell::s_ConstantCellCache;

/*********************************************************************************************************************/

LatticeCell* LatticeCell::GetValue(ConstantInt* v)
{
	auto it = s_ConstantCellCache.find(v);

	if (it != s_ConstantCellCache.end())
		return it->second;

	LatticeCell* cell = new LatticeCell(v);
	s_ConstantCellCache[v] = cell;

	return cell;
}

/*********************************************************************************************************************/

static LatticeCell* Meet(LatticeCell* a, LatticeCell* b)
{
	if (b->IsTop())
		return a;

	if (b->IsBottom())
		return LatticeCell::GetBottom();

	return (a == b) ? b : LatticeCell::GetBottom();
}

/*********************************************************************************************************************/

static LatticeCell* Meet(const std::vector<LatticeCell*>& cells)
{
	helix_assert(!cells.empty(), "Cannot meet empty list of lattice cells");

	if (cells.size() == 1)
		return cells[0];

	LatticeCell* cell = Meet(cells[0], cells[1]);

	for (size_t i = 2; i < cells.size(); ++i)
		cell = Meet(cell, cells[i]);

	return cell;
}

/*********************************************************************************************************************/

static void ConstructNodeGraph(Function* fn, std::vector<Node>& nodes)
{
	struct BlockInfo
	{
		size_t startIndex = 0;
		size_t endIndex   = 0;
	};

	size_t node_index = 0;

	std::unordered_map<BasicBlock*, BlockInfo> blocks_info;
	std::set<VirtualRegisterName*> all_virtual_registers;

	for (BasicBlock& bb : fn->blocks()) {
		const size_t block_start_index = node_index;

		for (Instruction& insn : bb) {
			nodes.emplace_back(&insn, node_index);
			node_index++;

			for (size_t operand_index = 0; operand_index < insn.GetCountOperands(); ++operand_index) {
				Value* operand = insn.GetOperand(operand_index);

				if (VirtualRegisterName* vreg = value_cast<VirtualRegisterName>(operand))
					all_virtual_registers.insert(vreg);
			}
		}

		const BlockInfo block_info { block_start_index, node_index };
		blocks_info[&bb] = block_info;
	}

	for (auto& [block, info] : blocks_info) {
		const std::vector<BasicBlock*> blockPredecessors = IR::GetPredecessors(block);

		for (BasicBlock* block_pred : blockPredecessors) {
			const size_t pred_node_index = blocks_info[block_pred].endIndex - 1;

			nodes[info.startIndex].AddPredecessor(pred_node_index);
		}
	}

	for (size_t i = 1; i < nodes.size(); ++i)
		nodes[i].AddPredecessor(i - 1);

	VariableMap* entryNodeInputs = nodes[0].GetInputs();

	for (VirtualRegisterName* vreg : all_virtual_registers)
		entryNodeInputs->Set(vreg, LatticeCell::GetBottom());
}

/*********************************************************************************************************************/

static VariableMap ComputeInputs(const std::vector<Node>& nodes, Node* node)
{
	VariableMap result = *node->GetInputs();

	std::unordered_map<VirtualRegisterName*, std::vector<LatticeCell*>> all;

	for (size_t pred_index : node->predecessors()) {
		const Node* pred_node = &nodes[pred_index];

		const VariableMap* prev_node_outputs = pred_node->GetOutputs();

		for (auto& [var, cell] : prev_node_outputs->vars()) {
			all[var].push_back(cell);
		}
	}

	for (auto& [var, cells] : all) {
		result.Set(var, Meet(cells));
	}

	return result;
}

/*********************************************************************************************************************/

static ConstantInt* EvaluateValueToConstant(Node* node, Value* value)
{
	if (ConstantInt* c = value_cast<ConstantInt>(value))
		return c;

	if (VirtualRegisterName* var = value_cast<VirtualRegisterName>(value)) {
		VariableMap* inputs = node->GetInputs();

		LatticeCell* cell = inputs->Get(var);

		if (cell->IsConstant())
			return cell->GetConstant();
	}

	return nullptr;
}

/*********************************************************************************************************************/

static ConstantInt* FoldConstantBinaryOperation(HLIR::Opcode opc, ConstantInt* lhs, ConstantInt* rhs)
{
	helix_assert(lhs->GetType() == rhs->GetType(), "LHS and RHS types must be the same in order to fold binop :)");

	Helix::Integer result = 0;

	/* #FIXME: Handle overflow for differently sized types correctly :) */
	switch (opc) {
	case HLIR::IAdd:  result = lhs->GetIntegralValue() + rhs->GetIntegralValue(); break;
	case HLIR::ISub:  result = lhs->GetIntegralValue() - rhs->GetIntegralValue(); break;
	case HLIR::IMul:  result = lhs->GetIntegralValue() * rhs->GetIntegralValue(); break;

		/* #FIXME: Add support for signed/unsigned division */
	default:
		return nullptr;
	}

	return ConstantInt::Create(lhs->GetType(), result);
}

/*********************************************************************************************************************/

static void ComputeOutputs(Node* node)
{
	VariableMap* OutputsPtr = node->GetOutputs();

	for (const auto& [var, cell] : node->GetInputs()->vars()) {
		OutputsPtr->Set(var, cell);
	}

	Instruction* insn = node->GetInsn();

	if (insn->GetOpcode() == HLIR::Set) {
		SetInsn* set = (SetInsn*)insn;

		VirtualRegisterName* var = value_cast<VirtualRegisterName>(set->GetRegister());
		helix_assert(var, "LHS of set is not a virtual register :(");

		Value* rhs = set->GetNewValue();

		if (ConstantInt* constant = value_cast<ConstantInt>(rhs)) {
			OutputsPtr->Set(var, LatticeCell::GetValue(constant));
			return;
		}
		else if (VirtualRegisterName* var_rhs = value_cast<VirtualRegisterName>(rhs)) {
			VariableMap* inputs = node->GetInputs();
			LatticeCell* cell = inputs->Get(var_rhs);

			OutputsPtr->Set(var, cell);
			return;
		}
	}

	if (HLIR::IsBinaryOp((HLIR::Opcode)insn->GetOpcode())) {
		BinOpInsn* binop = (BinOpInsn*)insn;

		ConstantInt* lhs = EvaluateValueToConstant(node, binop->GetLHS());
		ConstantInt* rhs = EvaluateValueToConstant(node, binop->GetRHS());

		if (lhs && rhs) {
			ConstantInt* result = FoldConstantBinaryOperation((HLIR::Opcode)binop->GetOpcode(), lhs, rhs);

			if (result) {
				OutputsPtr->Set(binop->GetResult(), LatticeCell::GetValue(result));
				return;
			}
		}
		else {
			OutputsPtr->Set(binop->GetResult(), LatticeCell::GetBottom());
			return;
		}
	}

	for (size_t op_index = 0; op_index < insn->GetCountOperands(); ++op_index) {
		if (insn->OperandHasFlags(op_index, Instruction::OP_WRITE)) {
			Value* op = insn->GetOperand(op_index);

			if (VirtualRegisterName* var = value_cast<VirtualRegisterName>(op))
				OutputsPtr->Set(var, LatticeCell::GetBottom());
		}
	}
}

/*********************************************************************************************************************/

static void PrintNode(SlotTracker& slots, const Node& node)
{
	char buf[128] = {};
	TextOutputStream tos(buf, 128);
	Helix::Print(slots, tos, *node.GetInsn());

	fmt::print("{}) {}\n", node.GetIndex(), buf);

	fmt::print("  Preds: ");

	for (size_t pred : node.predecessors())
		fmt::print("{}, ", pred);
	fmt::print("\n");

	fmt::print("  Inputs:  ");

	for (auto& [var, cell] : node.GetInputs()->vars()) {
		fmt::print("%{} = ", slots.GetValueSlot(var));

		if (cell->IsTop()) fmt::print("T");
		if (cell->IsBottom()) fmt::print("B");
		if (cell->IsConstant()) fmt::print("{}", cell->GetConstant()->GetIntegralValue());

		fmt::print(", ");
	}

	fmt::print("\n");

	fmt::print("  Outputs: ");

	for (auto& [var, cell] : node.GetOutputs()->vars()) {
		fmt::print("%{} = ", slots.GetValueSlot(var));

		if (cell->IsTop()) fmt::print("T");
		if (cell->IsBottom()) fmt::print("B");
		if (cell->IsConstant()) fmt::print("{}", cell->GetConstant()->GetIntegralValue());

		fmt::print(", ");
	}

	fmt::print("\n");
}

/*********************************************************************************************************************/

void SCP::Execute(Function* fn, const PassRunInformation&)
{
	// Construct node graph
	std::vector<Node> nodes;
	ConstructNodeGraph(fn, nodes);

	// Propagate constants

	std::vector<Node*> worklist;
	worklist.reserve(nodes.size());

	//for (int i = (int) nodes.size() - 1; i >= 0; i--)
	for (int i = 0; i < nodes.size(); ++i)
		worklist.push_back(&nodes[i]);

	SlotTracker slots;
	slots.CacheFunction(fn);

	while (!worklist.empty()) {
		Node* node = worklist.front();
		worklist.erase(worklist.begin());

		const VariableMap temp = ComputeInputs(nodes, node);

		if (temp != *node->GetInputs()) {
			node->SetInputs(temp);

			for (size_t pred_index : node->predecessors()) {
				worklist.push_back(&nodes[pred_index]);
			}

			worklist.push_back(node);
		}

		ComputeOutputs(node);
		//PrintNode(slots, *node);
	}

#if 0
	for (size_t i = 0; i < nodes.size(); ++i) {
		Node& node = nodes[i];
		PrintNode(slots, node);
	}
#endif

	// Rewrite
	for (Node& node : nodes) {
		Instruction* insn = node.GetInsn();

		if (HLIR::IsBinaryOp((HLIR::Opcode)insn->GetOpcode())) {
			BinOpInsn* binop = (BinOpInsn*)insn;
			VirtualRegisterName* result = binop->GetResult();
			
			VariableMap* outputs = node.GetOutputs();

			LatticeCell* cell = outputs->Get(result);

			if (cell->IsConstant()) {
				SetInsn* set = Helix::CreateSetInsn(result, cell->GetConstant());
				IR::ReplaceInstructionAndDestroyOriginal(binop, set);
			}
		}
		
		VariableMap* inputs = node.GetInputs();

		for (size_t op_index = 0; op_index < insn->GetCountOperands(); ++op_index) {
			if (insn->OperandHasFlags(op_index, Instruction::OP_READ)) {
				Value* op = insn->GetOperand(op_index);

				if (VirtualRegisterName* var = value_cast<VirtualRegisterName>(op)) {
					LatticeCell* cell = inputs->Get(var);

					if (cell->IsConstant()) {
						insn->SetOperand(op_index, cell->GetConstant());
					}
				}
			}
		}
	}
}

/*********************************************************************************************************************/
