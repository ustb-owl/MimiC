#ifndef MIMIC_BACK_ASM_MIR_PASSES_COLORING_H_
#define MIMIC_BACK_ASM_MIR_PASSES_COLORING_H_

#include <algorithm>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <ostream>
#include <cassert>
#include <cstddef>

#include "back/asm/mir/passes/regalloc.h"
#include "utils/hashing.h"

namespace mimic::back::asmgen {

/*
  graph coloring register allocator
  reference: http://kodu.ut.ee/~varmo/TM2010/slides/tm-reg.pdf
*/
class GraphColoringRegAllocPass : public RegAllocatorBase {
 public:
  GraphColoringRegAllocPass() {}

  void RunOn(const OprPtr &func_label, InstPtrList &insts) override {
    // perform allocation
    RunAllocator(func_label);
    // apply colored nodes
    for (const auto &[vreg, reg] : colored_nodes_) {
      AllocateVRegTo(vreg, reg);
    }
  }

  void AddAvaliableTempReg(const OprPtr &reg) override {
    avaliable_temps_.push_back(reg);
  }

  void AddAvaliableReg(const OprPtr &reg) override {
    avaliable_regs_.push_back(reg);
  }

  // setter
  void set_func_if_graphs(const FuncIfGraphs *func_if_graphs) {
    func_if_graphs_ = func_if_graphs;
  }

 private:
  struct NodeInfo {
    OprPtr node;
    std::size_t degree;
  };

  // check if the specific node is spilled
  bool IsNodeSpilled(const OprPtr &node) {
    assert(node->IsVirtual());
    auto vreg = static_cast<VirtRegOperand *>(node.get());
    return vreg->alloc_to() && vreg->alloc_to()->IsSlot();
  }

  // spill node to stack
  void SpillNode(const OprPtr &func_label, const OprPtr &node) {
    AllocateVRegTo(node, allocator().AllocateSlot(func_label));
  }

  // get interference graph of the specific function
  const IfGraph &GetIfGraph(const OprPtr &func_label) {
    auto it = func_if_graphs_->find(func_label);
    assert(it != func_if_graphs_->end());
    return it->second;
  }

  void PutMinDegToBack(std::vector<NodeInfo *> &nodes) {
    if (nodes.size() <= 1) return;
    auto it = std::min_element(nodes.begin(), nodes.end(),
                               [](const NodeInfo *n1, const NodeInfo *n2) {
                                 return n1->degree < n2->degree;
                               });
    std::swap(nodes.back(), *it);
  }

  void RebuildNodeStack(const OprPtr &func_label) {
    std::unordered_map<OprPtr, NodeInfo> all_nodes;
    std::vector<NodeInfo *> candidate;
    const auto &graph = GetIfGraph(func_label);
    // initialize all candidate nodes
    for (const auto &[node, info] : graph) {
      if (!IsNodeSpilled(node)) {
        // add all unspilled nodes to candidate node list
        std::size_t degree = 0;
        for (const auto &n : info.neighbours) {
          if (!IsNodeSpilled(n)) ++degree;
        }
        auto it = all_nodes.insert({node, {node, degree}}).first;
        candidate.push_back(&it->second);
      }
    }
    PutMinDegToBack(candidate);
    // rebuild node stack
    nodes_.clear();
    while (!candidate.empty()) {
      // push the node with the minimum degree into stack
      auto ni = candidate.back();
      nodes_.push_back(ni->node);
      // remove node from graph
      candidate.pop_back();
      ni->degree = 0;
      // update degree of all its neighbours
      auto it = graph.find(ni->node);
      assert(it != graph.end());
      for (const auto &n : it->second.neighbours) {
        auto an_it = all_nodes.find(n);
        if (an_it != all_nodes.end()) {
          --an_it->second.degree;
        }
      }
      // update candidate node list
      PutMinDegToBack(candidate);
    }
  }

  // set color of the specific node
  void SetNodeColor(const OprPtr &node, const OprPtr &color,
                    std::unordered_set<OprPtr> &used_color) {
    colored_nodes_[node] = color;
    used_color.insert(color);
  }

  // colorize the specific node
  bool ColorizeNode(const OprPtr &node, const IfGraphNodeInfo &info,
                    std::unordered_set<OprPtr> &used_color) {
    // try to use the hint colors
    for (const auto &n : info.suggest_same) {
      auto it = colored_nodes_.find(n);
      const auto &color = it->second;
      if (it != colored_nodes_.end() && !used_color.count(color)) {
        if (IsTempReg(color) && !info.can_alloc_temp) continue;
        SetNodeColor(node, color, used_color);
        return true;
      }
    }
    // try to colorize with temporary registers
    if (info.can_alloc_temp) {
      for (const auto &i : avaliable_temps_) {
        if (!used_color.count(i)) {
          SetNodeColor(node, i, used_color);
          return true;
        }
      }
    }
    // colorize with avaliable registers
    for (const auto &i : avaliable_regs_) {
      if (!used_color.count(i)) {
        SetNodeColor(node, i, used_color);
        return true;
      }
    }
    return false;
  }

  // choose an uncolored node from stack and spill it
  void ChooseAndSpill(const OprPtr &func_label) {
    const auto &graph = GetIfGraph(func_label);
    auto get_degree = [&graph](const OprPtr &node) {
      auto it = graph.find(node);
      assert(it != graph.end());
      return it->second.neighbours.size();
    };
    auto compare = [&get_degree](const OprPtr &l, const OprPtr &r) {
      return l->use_count() * get_degree(r) <
             r->use_count() * get_degree(l);
    };
    auto it = std::min_element(nodes_.begin(), nodes_.end(), compare);
    SpillNode(func_label, *it);
  }

  // colorize all nodes in stack
  bool Colorize(const OprPtr &func_label) {
    const auto &graph = GetIfGraph(func_label);
    while (!nodes_.empty()) {
      // get node info from original graph
      auto it = graph.find(nodes_.back());
      assert(it != graph.end());
      const auto &info = it->second;
      // get color of all it's neighbours
      std::unordered_set<OprPtr> used_color;
      for (const auto &n : info.neighbours) {
        auto cn_it = colored_nodes_.find(n);
        if (cn_it != colored_nodes_.end()) {
          used_color.insert(cn_it->second);
        }
      }
      // try to colorize
      if (!ColorizeNode(nodes_.back(), info, used_color)) {
        // can not be colorized, spill to stack and exit
        ChooseAndSpill(func_label);
        return false;
      }
      // pop the node from stack
      nodes_.pop_back();
    }
    return true;
  }

  // perform register allocation
  void RunAllocator(const OprPtr &func_label) {
    bool stop = false;
    while (!stop) {
      colored_nodes_.clear();
      // rebuild node stack
      RebuildNodeStack(func_label);
      // try to colorize
      stop = Colorize(func_label);
    }
  }

  // dump interference graph
  void DumpGraph(std::ostream &os, const OprPtr &func_label) {
    std::unordered_set<std::pair<OprPtr, OprPtr>> visited_edge;
    os << "graph if_graph {" << std::endl;
    for (const auto &[node, info] : GetIfGraph(func_label)) {
      auto node_ptr = static_cast<VirtRegOperand *>(node.get());
      os << "  n" << node_ptr->id();
      os << " [label = \"";
      node_ptr->alloc_to()->Dump(os);
      os << "\"]" << std::endl;
      for (const auto &n : info.neighbours) {
        if (visited_edge.insert({node, n}).second) {
          visited_edge.insert({n, node});
          auto n_ptr = static_cast<VirtRegOperand *>(n.get());
          os << "  n" << n_ptr->id();
          os << " [label = \"";
          n_ptr->alloc_to()->Dump(os);
          os << "\"]" << std::endl;
          os << "  n" << node_ptr->id() << " -- n";
          os << n_ptr->id() << std::endl;
        }
      }
    }
    os << "}" << std::endl;
  }

  // all avaliable registers
  std::vector<OprPtr> avaliable_temps_, avaliable_regs_;
  // reference of interference graph
  const FuncIfGraphs *func_if_graphs_;
  // stack of nodes that need to be colored
  std::vector<OprPtr> nodes_;
  // all colored nodes (node -> reg)
  std::unordered_map<OprPtr, OprPtr> colored_nodes_;
};

}  // namespace mimic::back::asmgen

#endif  // MIMIC_BACK_ASM_MIR_PASSES_COLORING_H_
