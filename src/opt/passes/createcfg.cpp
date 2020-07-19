#include <iostream>
#include <vector>

#include "mid/module.h"
#include "mid/ssa.h"
#include "opt/pass.h"
#include "opt/passman.h"

using namespace mimic::opt;
using namespace mimic::mid;
using namespace mimic::define;

namespace {

class Edge;
using EdgePtr = std::shared_ptr<Edge>;
using EdgeList = std::vector<Edge>;

#ifndef __FILENAME__
#define __FILENAME__ __FILE__
#endif

#define LOG() \
  std::cout << '[' << __FILENAME__ << ":" << std::dec << __LINE__ << "] "

class Edge {
 public:
  Edge(int vertex) : vertex_(vertex) {}

 private:
  int vertex_;
};

class CFGNode {
 public:
  CFGNode(const BlockPtr basicblock) : basicBlock_(basicblock) {}
  //   CFGNode(const BlockPtr basicblock, const EdgePtr firstedge)
  //   : basicBlock_(basicblock), firstEdge_(firstedge) {}
  // copy constructor
  CFGNode(const CFGNode &node) {
    basicBlock_ = node.basicBlock_;
    edgeList_ = node.edgeList_;
  }
  // move sonstructor
  CFGNode(CFGNode &&node) noexcept
      : basicBlock_(std::move(node.basicBlock_)),
        edgeList_(std::move(node.edgeList_)) {}

  BlockPtr &GetBlockPtr() { return basicBlock_; }

 private:
  BlockPtr basicBlock_;
  EdgeList edgeList_;
};

class CFG {
 public:
  CFG() {}
  void Insert(const CFGNode &node) { nodeList_.push_back(node); }
  std::vector<CFGNode> GetNodeList() { return nodeList_; }
  int NodeNum() { return nodeList_.size(); }

 private:
  std::vector<CFGNode> nodeList_;
};

class CreateCFG : public FunctionPass {
 public:
  bool RunOnFunction(const UserPtr &func) override {
    LOG() << "Function ssa size:\t" << func->size() << std::endl;
    for (const auto &i : *func) {
      i.value()->RunPass(*this);
    }

    // auto uses = bpList[0]->uses();
    Itercfg();
    // LOG() << bpList[0]->name() << std::endl;
    return false;
  }

  void RunOn(BlockSSA &ssa) override {
    BlockPtr bp = std::make_shared<BlockSSA>(ssa);
    LOG() << bp->name() << std::endl;

    bpList.push_back(bp);
    cfg.Insert(CFGNode(bp));
  }

  void Itercfg() {
    for (auto &i : cfg.GetNodeList()) {
      BlockPtr bp = i.GetBlockPtr();
      LOG() << "Block Name:\t" << bp->name() << "\nBlock preds Num: \t"
            << bp->size() << std::endl;
    }
  }

 private:
  std::vector<BlockPtr> bpList;
  CFG cfg;
};
}  // namespace

REGISTER_PASS(CreateCFG, create_cfg, 0, true);
