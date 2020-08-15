#ifndef MIMIC_BACK_ASM_ARCH_AARCH32_PASSES_INSTSCHED_H_
#define MIMIC_BACK_ASM_ARCH_AARCH32_PASSES_INSTSCHED_H_

#include <unordered_map>
#include <unordered_set>
#include <ostream>
#include <sstream>
#include <vector>
#include <functional>
#include <queue>
#include <cassert>
#include <cstddef>

#include "back/asm/mir/pass.h"
#include "back/asm/arch/aarch32/instdef.h"

namespace mimic::back::asmgen::aarch32 {

/*
  this pass will perform simple instruction scheduling
  based on topological sorting
*/
class InstSchedulingPass : public PassInterface {
 public:
  InstSchedulingPass() {}

  void RunOn(const OprPtr &func_label, InstPtrList &insts) override {
    InstPtrList new_insts;
    for (const auto &i : insts) {
      auto inst = static_cast<AArch32Inst *>(i.get());
      switch (inst->opcode()) {
        case OpCode::ADD: case OpCode::SUB: case OpCode::SUBS:
        case OpCode::RSB: case OpCode::MUL: case OpCode::MLS:
        case OpCode::SDIV: case OpCode::UDIV: case OpCode::MOV:
        case OpCode::MOVW: case OpCode::MOVT: case OpCode::MVN:
        case OpCode::AND: case OpCode::ORR: case OpCode::EOR:
        case OpCode::LSL: case OpCode::LSR: case OpCode::ASR:
        case OpCode::CLZ: case OpCode::SXTB: case OpCode::UXTB: {
          preds_.insert({i, {}});
          for (const auto &opr : inst->oprs()) {
            AddPred(i, opr.value());
            UpdateDef(opr.value(), i);
          }
          assert(inst->dest());
          AddPred(i, inst->dest());
          UpdateDef(inst->dest(), i);
          break;
        }
        case OpCode::LDR: case OpCode::LDRB: {
          // special handling for load instructions
          preds_.insert({i, {}});
          // memory address
          AddPred(i, inst->oprs()[0].value());
          UpdateDef(inst->oprs()[0].value(), i);
          // destination
          AddPred(i, inst->dest());
          UpdateDef(inst->dest(), i);
          // last store instruction
          AddLastStoreAsPred(i);
          break;
        }
        case OpCode::STR: case OpCode::STRB: {
          // special handling for store instructions
          preds_.insert({i, {}});
          // memory address & value
          for (const auto &opr : inst->oprs()) {
            AddPred(i, opr.value());
            UpdateDef(opr.value(), i);
          }
          // last store instruction
          AddLastStoreAsPred(i);
          last_store_ = i;
          break;
        }
        default: {
          // instructions that can not be scheduled (barrier)
          if (!preds_.empty()) {
            // get scheduled instruction sequence
            FindRemaining();
            auto sched_insts = ListSchedule();
            // emit scheduled instructions
            for (const auto &i : sched_insts) {
              new_insts.insert(new_insts.end(), i.begin(), i.end());
            }
          }
          // emit current instruction
          new_insts.push_back(i);
          // reset for next schedule
          ResetDefs();
          break;
        }
      }
    }
    // replace with scheduled instruction sequence
    insts = std::move(new_insts);
  }

 private:
  using OpCode = AArch32Inst::OpCode;
  using ShiftOp = AArch32Inst::ShiftOp;
  // instruction map
  template <typename T>
  using InstMap = std::unordered_map<InstPtr, T>;
  // instruction queue
  using InstQueue = std::priority_queue<
      InstPtr, std::vector<InstPtr>,
      std::function<bool(const InstPtr &, const InstPtr &)>>;
  // instruction set for each cycle
  using InstSet = std::vector<std::vector<InstPtr>>;

  // latency of instructions
  // reference: Cortex-A72 Software Optimization Guide
  std::size_t GetLatency(const InstPtr &inst) {
    auto inst_ptr = static_cast<AArch32Inst *>(inst.get());
    auto opcode = inst_ptr->opcode();
    switch (opcode) {
      case OpCode::STR: case OpCode::STRB: case OpCode::ADD:
      case OpCode::SUB: case OpCode::SUBS: case OpCode::RSB:
      case OpCode::MOV: case OpCode::MOVW: case OpCode::MOVT:
      case OpCode::MVN: case OpCode::AND: case OpCode::ORR:
      case OpCode::EOR: case OpCode::LSL: case OpCode::LSR:
      case OpCode::ASR: case OpCode::CLZ: case OpCode::SXTB:
      case OpCode::UXTB: return 1 + (inst_ptr->shift_op() != ShiftOp::NOP);
      case OpCode::MUL: case OpCode::MLS: return 3;
      case OpCode::LDR: case OpCode::LDRB: return 4;
      case OpCode::SDIV: case OpCode::UDIV: return 12;
      default: assert(false); return 0;
    }
  }

  void ResetDefs() {
    defs_.clear();
    last_store_ = nullptr;
    preds_.clear();
    remaining_.clear();
  }

  void AddPred(const InstPtr &inst, const OprPtr &val) {
    std::unordered_map<OprPtr, InstPtr>::iterator it;
    if (val->IsSlot()) {
      auto slot = static_cast<AArch32Slot *>(val.get());
      it = defs_.find(slot->base());
    }
    else {
      it = defs_.find(val);
    }
    if (it != defs_.end() && it->second != inst) {
      preds_[inst].insert(it->second);
    }
  }

  void AddLastStoreAsPred(const InstPtr &inst) {
    if (last_store_) preds_[inst].insert(last_store_);
  }

  void UpdateDef(const OprPtr &dest, const InstPtr &inst) {
    if (dest->IsReg()) {
      defs_[dest] = inst;
    }
    else if (dest->IsSlot()) {
      auto slot = static_cast<AArch32Slot *>(dest.get());
      defs_[slot->base()] = inst;
    }
  }

  // dump dependency graph of instructions
  void DumpInstGraph(std::ostream &os) {
    InstMap<std::size_t> ids;
    std::size_t next_id = 0;
    // determine id of the specific node
    auto get_id = [&ids, &next_id](const InstPtr &i) {
      auto it = ids.find(i);
      if (it != ids.end()) {
        return it->second;
      }
      else {
        ids.insert({i, next_id});
        return next_id++;
      }
    };
    // dump header
    os << "digraph inst_graph {" << std::endl;
    os << "  node [shape = record]" << std::endl;
    // dump predecessors
    for (const auto &[i, preds] : preds_) {
      auto id = get_id(i);
      os << std::endl;
      // dump current node
      std::ostringstream oss;
      i->Dump(oss);
      os << "  n" << id << " [label = \"<inst> ";
      os << oss.str().substr(1, oss.str().size() - 2) << " | ";
      os << "<rem> " << GetRemaining(i) << "\"]" << std::endl;
      // dump edges
      for (const auto &p : preds) {
        os << "  n" << get_id(p) << " -> n" << id;
        os << " [label = \"" << GetLatency(p);
        os << "\"]" << std::endl;
      }
    }
    os << "}" << std::endl << std::endl;
  }

  // determine the minimum remaining cycle of each instruction
  // reverse traversing
  void FindRemaining() {
    InstMap<std::size_t> count;
    // initialize 'count' & 'remaining' map
    for (const auto &[i, _] : preds_) {
      count[i] = 0;
      remaining_[i] = GetLatency(i);
    }
    // initialize 'count' map with in-degree of nodes (reversed graph)
    for (const auto &[head, p] : preds_) {
      for (const auto &tail : p) {
        ++count[tail];
      }
    }
    // initialize worklist
    std::vector<InstPtr> worklist;
    for (const auto &[i, c] : count) {
      if (!c) worklist.push_back(i);
    }
    // update remaining cycle
    while (!worklist.empty()) {
      auto inst = worklist.back();
      worklist.pop_back();
      auto r = remaining_[inst];
      for (const auto &p : preds_[inst]) {
        --count[p];
        auto new_rem = r + GetLatency(p);
        if (new_rem > remaining_[p]) remaining_[p] = new_rem;
        if (!count[p]) worklist.push_back(p);
      }
    }
  }

  // get remaining cycle of the specific instruction
  std::size_t GetRemaining(const InstPtr &inst) const {
    auto it = remaining_.find(inst);
    assert(it != remaining_.end());
    return it->second;
  }

  // perform list schedule
  InstSet ListSchedule() {
    constexpr std::size_t kMaxCycle = 13;
    InstMap<std::size_t> count, earliest;
    // initialize 'count' & 'earliest' map
    for (const auto &[i, _] : preds_) {
      count[i] = 0;
      earliest[i] = 0;
    }
    // initialize 'count' map with in-degree of nodes
    // and get successor map
    InstMap<std::vector<InstPtr>> succs;
    for (const auto &[head, p] : preds_) {
      for (const auto &tail : p) {
        ++count[head];
        succs[tail].push_back(head);
      }
    }
    // initialize worklist
    std::vector<InstQueue> worklist;
    worklist.resize(kMaxCycle,
                    InstQueue([this](const InstPtr &l, const InstPtr &r) {
                      return GetRemaining(l) < GetRemaining(r);
                    }));
    std::size_t inst_count = 0;
    for (const auto &[i, c] : count) {
      if (!c) {
        worklist[0].push(i);
        ++inst_count;
      }
    }
    // perform schedule
    std::size_t cur_cycle = 0, cur_wl = 0;
    InstSet insts = {{}};
    while (inst_count > 0) {
      // switch worklist
      while (worklist[cur_wl].empty()) {
        ++cur_cycle;
        insts.push_back({});
        cur_wl = (cur_wl + 1) % kMaxCycle;
      }
      // update instruction list
      while (!worklist[cur_wl].empty()) {
        // pick instruction from current worklist
        auto inst = worklist[cur_wl].top();
        worklist[cur_wl].pop();
        // add to instruction list
        insts[cur_cycle].push_back(inst);
        --inst_count;
        // check for successors
        for (const auto &s : succs[inst]) {
          --count[s];
          // update 'earliest' map
          auto new_ear = cur_cycle + GetLatency(inst);
          if (new_ear > earliest[s]) earliest[s] = new_ear;
          // try to add to worklist
          if (!count[s]) {
            auto cycle = earliest[s] % kMaxCycle;
            worklist[cycle].push(s);
            ++inst_count;
          }
        }
      }
    }
    return insts;
  }

  std::unordered_map<OprPtr, InstPtr> defs_;
  InstPtr last_store_;
  InstMap<std::unordered_set<InstPtr>> preds_;
  InstMap<std::size_t> remaining_;
};

}  // namespace mimic::back::asmgen::aarch32

#endif  // MIMIC_BACK_ASM_ARCH_AARCH32_PASSES_INSTSCHED_H_
