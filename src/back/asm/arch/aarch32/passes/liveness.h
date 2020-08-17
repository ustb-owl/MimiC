#ifndef MIMIC_BACK_ASM_ARCH_AARCH32_PASSES_LIVENESS_H_
#define MIMIC_BACK_ASM_ARCH_AARCH32_PASSES_LIVENESS_H_

#include <vector>
#include <list>
#include <unordered_map>
#include <unordered_set>
#include <cstddef>
#include <cassert>

#include "back/asm/mir/pass.h"
#include "back/asm/mir/label.h"
#include "back/asm/arch/aarch32/instdef.h"
#include "back/asm/mir/passes/regalloc.h"

namespace mimic::back::asmgen::aarch32 {

/*
  liveness analysis on MIR (aarch32 architecture)
  this pass will:
  1.  calculate the CFG of input function
  2.  analysis liveness information of all virtual registers in function
*/
class LivenessAnalysisPass : public PassInterface {
 public:
  // liveness information type
  enum class LivenessInfoType {
    LiveIntervals, InterferenceGraph,
  };

  LivenessAnalysisPass(LivenessInfoType info_type,
                       TempRegChecker temp_checker,
                       const RegList &temp_regs,
                       const RegList &temp_regs_with_lr,
                       const RegList &regs)
      : info_type_(info_type), temp_checker_(temp_checker),
        temp_regs_(temp_regs), temp_regs_with_lr_(temp_regs_with_lr),
        regs_(regs) {}

  void RunOn(const OprPtr &func_label, InstPtrList &insts) override {
    Reset();
    BuildCFG(insts);
    InitDefUseInfo();
    RunLivenessAnalysis();
    // generate avaliable registers
    GenerateAvaliableRegs(func_label, insts);
    // generate liveness info
    if (info_type_ == LivenessInfoType::LiveIntervals) {
      GenerateLiveIntervals(func_label);
    }
    else {
      assert(info_type_ == LivenessInfoType::InterferenceGraph);
      GenerateInterferenceGraph(func_label);
    }
  }

  // getters
  const FuncRegList &func_temp_regs() const { return func_temp_regs_; }
  const FuncRegList &func_regs() const { return func_regs_; }
  const FuncLiveIntervals &func_live_intervals() const {
    return func_live_intervals_;
  }
  const FuncIfGraphs &func_if_graphs() const { return func_if_graphs_; }

 private:
  using OpCode = AArch32Inst::OpCode;
  using BlockId = std::size_t;

  // representation of basic block
  struct BasicBlock {
    // instructions in current basic block
    InstPtrList insts;
    // label of predecessors
    std::vector<BlockId> preds;
    // label of successors
    std::vector<BlockId> succs;
    // all defined (killed) virtual registers
    std::unordered_set<OprPtr> var_kill;
    // all upward-exposed virtual registers
    std::unordered_set<OprPtr> ue_var;
    // for liveness analysis
    std::unordered_set<OprPtr> live_out;
  };

  // reset internal status
  void Reset() {
    labels_.clear();
    next_bid_ = 0;
    bbs_.clear();
    order_.clear();
  }

  // get block id of label, or assign a new id for the specific label
  BlockId GetBlockId(const OperandBase *label) {
    auto it = labels_.find(label);
    if (it != labels_.end()) {
      return it->second;
    }
    else {
      return labels_[label] = ++next_bid_;
    }
  }

  // get next block id for anonymous basic block
  BlockId GetBlockId() {
    return ++next_bid_;
  }

  // get pointer of next instruction of the specific position
  AArch32Inst *GetNextInst(const InstPtrList &insts,
                           InstPtrList::const_iterator it) {
    if (++it == insts.end()) return nullptr;
    return static_cast<AArch32Inst *>(it->get());
  }

  // get pointer of previous instruction of the specific position
  AArch32Inst *GetPrevInst(const InstPtrList &insts,
                           InstPtrList::const_iterator it) {
    if (it-- == insts.begin()) return nullptr;
    return static_cast<AArch32Inst *>(it->get());
  }

  // check if the specific opcode represents to a branch
  bool IsBranch(OpCode op) {
    switch (op) {
      case OpCode::BEQ: case OpCode::BNE: case OpCode::BLO:
      case OpCode::BLT: case OpCode::BLS: case OpCode::BLE:
      case OpCode::BHI: case OpCode::BGT: case OpCode::BHS:
      case OpCode::BGE: return true;
      default: return false;
    }
  }

  // build up CFG by traversing instruction list
  void BuildCFG(const InstPtrList &insts) {
    BlockId cur_bid = 0;
    order_.push_back(cur_bid);
    // traverse all instructions
    for (auto it = insts.begin(); it != insts.end(); ++it) {
      auto inst = static_cast<AArch32Inst *>(it->get());
      if (inst->opcode() == OpCode::LABEL) {
        // switch to new basic block
        auto label = inst->oprs()[0].value().get();
        auto next_bid = GetBlockId(label);
        // check prevoius instruction
        auto pi = GetPrevInst(insts, it);
        if (!pi ||
            (pi->opcode() != OpCode::B && pi->opcode() != OpCode::BX &&
             pi->opcode() != OpCode::POP)) {
          // update predecessor & successor
          bbs_[cur_bid].succs.push_back(next_bid);
          bbs_[next_bid].preds.push_back(cur_bid);
        }
        cur_bid = next_bid;
        order_.push_back(cur_bid);
      }
      else {
        // add instruction to current block
        auto &cur_bb = bbs_[cur_bid];
        cur_bb.insts.push_back(*it);
        // check for branch instructions
        if (IsBranch(inst->opcode())) {
          // update predecessor & successor
          auto bid = GetBlockId(inst->oprs()[0].value().get());
          cur_bb.succs.push_back(bid);
          bbs_[bid].preds.push_back(cur_bid);
          // check if next instruction is b/label
          auto opcode = GetNextInst(insts, it)->opcode();
          if (opcode != OpCode::B && opcode != OpCode::LABEL) {
            // split basic block
            auto next_bid = GetBlockId();
            cur_bb.succs.push_back(next_bid);
            bbs_[next_bid].preds.push_back(cur_bid);
            cur_bid = next_bid;
            order_.push_back(cur_bid);
          }
        }
        else if (inst->opcode() == OpCode::B) {
          // update predecessor & successor
          auto bid = GetBlockId(inst->oprs()[0].value().get());
          cur_bb.succs.push_back(bid);
          bbs_[bid].preds.push_back(cur_bid);
        }
      }
    }
  }

  // for debugging
  void DumpCFG(std::ostream &os) {
    auto dump_id_list = [&os](const std::vector<BlockId> &ids,
                              const char *name) {
      os << "  " << name << ": ";
      if (ids.empty()) {
        os << "<none>";
      }
      else {
        bool is_first = true;
        for (const auto &i : ids) {
          if (is_first) {
            is_first = false;
          }
          else {
            os << ", ";
          }
          os << i;
        }
      }
      os << std::endl;
    };
    auto dump_vregs = [&os](const std::unordered_set<OprPtr> &vregs,
                            const char *name) {
      os << "  " << name << ": ";
      if (vregs.empty()) {
        os << "<none>";
      }
      else {
        bool is_first = true;
        for (const auto &i : vregs) {
          if (is_first) {
            is_first = false;
          }
          else {
            os << ", ";
          }
          i->Dump(os);
        }
      }
      os << std::endl;
    };
    for (const auto &[bid, bb] : bbs_) {
      os << "block " << bid << ':' << std::endl;
      dump_id_list(bb.preds, "preds");
      dump_id_list(bb.succs, "succs");
      dump_vregs(bb.var_kill, "var_kill");
      dump_vregs(bb.ue_var, "ue_var");
      dump_vregs(bb.live_out, "live_out");
      for (const auto &i : bb.insts) i->Dump(os);
      os << std::endl;
    }
  }

  // initialize def/use information for all basic blocks
  void InitDefUseInfo() {
    for (auto &&[_, bb] : bbs_) {
      assert(bb.var_kill.empty() && bb.ue_var.empty());
      for (const auto &inst : bb.insts) {
        // initialize use info
        for (const auto &opr : inst->oprs()) {
          const auto &v = opr.value();
          if (v->IsVirtual() && !bb.var_kill.count(v)) {
            bb.ue_var.insert(v);
          }
        }
        // initialize def info
        if (inst->dest() && inst->dest()->IsVirtual()) {
          bb.var_kill.insert(inst->dest());
        }
      }
    }
  }

  // reverse post order traverse on reverse CFG
  void TraverseRPO(BlockId cur, std::list<BlockId> &rpo,
                   std::unordered_set<BlockId> &visited) {
    if (!visited.insert(cur).second) return;
    for (const auto &i : bbs_[cur].preds) {
      TraverseRPO(i, rpo, visited);
    }
    rpo.push_front(cur);
  }

  // get the block id sequence in RPO on reverse CFG
  std::list<BlockId> GetReversePostOrder() {
    // find entry node of reverse CFG (exit node of CFG)
    BlockId entry;
    for (const auto &[id, bb] : bbs_) {
      if (bb.succs.empty()) {
        entry = id;
        break;
      }
    }
    // perform reverse post order traverse
    std::list<BlockId> rpo;
    std::unordered_set<BlockId> visited;
    TraverseRPO(entry, rpo, visited);
    return rpo;
  }

  // run liveness analysis on current CFG
  void RunLivenessAnalysis() {
    auto rpo = GetReversePostOrder();
    // perform analysis
    bool changed = true;
    while (changed) {
      changed = false;
      // traverse basic blocks in RPO
      for (const auto &bid : rpo) {
        auto &bb = bbs_[bid];
        // LiveOut(bb) = union_{m in succs}
        //                     (UEVar(m) union
        //                      (LiveOut(m) inter bar(VarKill(m))))
        for (const auto &succ_bid : bb.succs) {
          const auto &succ = bbs_[succ_bid];
          for (const auto &vreg : succ.ue_var) {
            if (bb.live_out.insert(vreg).second) changed = true;
          }
          for (const auto &vreg : succ.live_out) {
            if (!succ.var_kill.count(vreg)) {
              if (bb.live_out.insert(vreg).second) changed = true;
            }
          }
        }
      }
    }
  }

  // generate avaliable registers
  void GenerateAvaliableRegs(const OprPtr &func_label,
                             const InstPtrList &insts) {
    // check if there is function call in current function
    bool has_call = false;
    for (const auto &i : insts) {
      if (i->IsCall()) {
        has_call = true;
        break;
      }
    }
    // initialize register list reference of current function
    func_temp_regs_[func_label] = has_call ? &temp_regs_with_lr_
                                           : &temp_regs_;
    func_regs_[func_label] = &regs_;
  }

  void LogLiveInterval(LiveIntervals &lis, const OprPtr &vreg,
                       std::size_t pos, std::size_t last_temp_pos) {
    assert(vreg->IsVirtual());
    // get live interval info
    auto it = lis.find(vreg);
    if (it != lis.end()) {
      // update end position
      auto &li = it->second;
      li.end_pos = pos;
      // check if there are usings of temporary register in interval
      if (last_temp_pos > li.start_pos) li.can_alloc_temp = false;
    }
    else {
      // add new live interval info
      lis.insert({vreg, {pos, pos, true}});
    }
  }

  // generate live intervals for linear scan register allocator
  void GenerateLiveIntervals(const OprPtr &func_label) {
    auto &live_intervals = func_live_intervals_[func_label];
    std::size_t pos = 0, last_temp_pos = 0;
    for (const auto &bid : order_) {
      const auto &bb = bbs_[bid];
      // traverse all instructions
      for (const auto &i : bb.insts) {
        for (const auto &opr : i->oprs()) {
          if (!opr.value()->IsVirtual()) continue;
          LogLiveInterval(live_intervals, opr.value(), pos, last_temp_pos);
        }
        if (i->dest() && i->dest()->IsReg()) {
          const auto &dest = i->dest();
          if (dest->IsVirtual()) {
            LogLiveInterval(live_intervals, dest, pos, last_temp_pos);
          }
          else if (temp_checker_(dest)) {
            // update 'last_temp_pos'
            last_temp_pos = pos;
          }
        }
        // update 'last_temp_pos' if current is a call instruction
        if (i->IsCall()) last_temp_pos = pos;
        ++pos;
      }
      // log virtual registers in 'live out' set
      for (const auto &vreg : bb.live_out) {
        LogLiveInterval(live_intervals, vreg, pos, last_temp_pos);
      }
    }
  }

  void AddEdge(IfGraph &if_graph, const OprPtr &n1, const OprPtr &n2) {
    if (n1 != n2) {
      if_graph[n1].neighbours.insert(n2);
      if_graph[n2].neighbours.insert(n1);
    }
    else {
      if_graph.insert({n1, {}});
    }
  }

  void AddSuggestSame(IfGraph &if_graph, const OprPtr &n1,
                      const OprPtr &n2) {
    if (!n1->IsVirtual() || !n2->IsVirtual()) return;
    if (n1 != n2) {
      if_graph[n1].suggest_same.insert(n2);
      if_graph[n2].suggest_same.insert(n1);
    }
    else {
      if_graph.insert({n1, {}});
    }
  }

  // generate interference graph for graph coloring register allocator
  void GenerateInterferenceGraph(const OprPtr &func_label) {
    auto &if_graph = func_if_graphs_[func_label];
    std::unordered_set<OprPtr> can_not_alloc_temp;
    // traverse all blocks
    for (const auto &bid : order_) {
      const auto &bb = bbs_[bid];
      auto live_now = bb.live_out;
      // traverse all instructions in reverse order
      for (auto it = bb.insts.rbegin(); it != bb.insts.rend(); ++it) {
        const auto &i = *it;
        // update 'can_not_alloc_temp'
        if ((i->dest() && temp_checker_(i->dest())) || i->IsCall()) {
          for (const auto &val : live_now) {
            can_not_alloc_temp.insert(val);
          }
        }
        // check for destination register
        if (i->dest() && i->dest()->IsVirtual()) {
          // add edges
          for (const auto &val : live_now) {
            AddEdge(if_graph, val, i->dest());
          }
          // remove from set
          live_now.erase(i->dest());
        }
        // add operands to set
        for (const auto &opr : i->oprs()) {
          if (!opr.value()->IsVirtual()) continue;
          live_now.insert(opr.value());
        }
        // update 'suggest_same'
        if (i->IsMove()) {
          const auto &dest = i->dest(), &src = i->oprs()[0].value();
          AddSuggestSame(if_graph, dest, src);
        }
      }
    }
    // apply 'can_alloc_temp' flag of all nodes in graph
    for (auto &&[vreg, info] : if_graph) {
      info.can_alloc_temp = !can_not_alloc_temp.count(vreg);
    }
  }

  // map of labels to basic block id
  std::unordered_map<const OperandBase *, BlockId> labels_;
  // next basic block id
  BlockId next_bid_;
  // all basic blocks, id of entry block is zero
  std::unordered_map<BlockId, BasicBlock> bbs_;
  // original order of all basic blocks
  std::vector<BlockId> order_;
  // liveness info type
  LivenessInfoType info_type_;
  // temporary register checker
  TempRegChecker temp_checker_;
  // architecture register list
  const RegList &temp_regs_, &temp_regs_with_lr_, &regs_;
  // avaliable registers of all functions
  FuncRegList func_temp_regs_, func_regs_;
  // live intervals of all functions
  FuncLiveIntervals func_live_intervals_;
  // interference graph of all functions
  FuncIfGraphs func_if_graphs_;
};

}  // namespace mimic::back::asmgen::aarch32

#endif  // MIMIC_BACK_ASM_ARCH_AARCH32_PASSES_LIVENESS_H_
