#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include <unordered_map> 
#include <unordered_set> 

using namespace llvm;

using Child_Parent_CFG = std::unordered_map<BasicBlock*, std::pair<std::vector<BasicBlock*>, std::vector<BasicBlock*>>>;

Child_Parent_CFG generate_child_parent_cfg(Function &F) {
  std::unordered_map<BasicBlock*, std::vector<BasicBlock*>> cfg_child_ptrs;
  std::unordered_map<BasicBlock*, std::vector<BasicBlock*>> cfg_parent_ptrs;

  for (auto it = F.begin(); it != F.end(); it++) {
    auto B = &(*it);
    const Instruction* t = B->getTerminator();
    int i;
    int numSuc = t->getNumSuccessors();
    // create a list of B's children
    std::vector<BasicBlock*> succs;
    for (i = 0; i < numSuc; i++) {
      BasicBlock* succ = t->getSuccessor(i);

      // get the successor and add B to that node's parent list
      auto succ_parents = cfg_parent_ptrs[succ];
      succ_parents.push_back(B);
      cfg_parent_ptrs[succ] = succ_parents;

      succs.push_back(succ);
    }
    cfg_child_ptrs[B] = succs;
  }

  Child_Parent_CFG cfg;

  for (auto it = cfg_child_ptrs.begin(); it != cfg_child_ptrs.end(); it++) {
    auto block = it->first;
    auto parents = cfg_parent_ptrs[block];
    auto children = it->second;
    std::pair<std::vector<BasicBlock*>, std::vector<BasicBlock*>> child_parent(children, parents);
    cfg[block] = child_parent;
  }

  return cfg;
}

BasicBlock* get_entry(Child_Parent_CFG cfg) {
  for (auto it = cfg.begin(); it != cfg.end(); it++) {
    auto parents = it->second.second;
    if (parents.size() == 0) {
      return it->first;
    }
  }
  throw "No entry block for cfg";
}

BasicBlock* get_exit(Child_Parent_CFG cfg) {
  for (auto it = cfg.begin(); it != cfg.end(); it++) {
    auto children = it->second.first;
    if (children.size() == 0) {
      return it->first;
    }
  }
  throw "No exit block for cfg";
}

Function* init_table_func = NULL;
Function* get_init_table_func(LLVMContext &context, Module* module) {
  if (init_table_func == NULL) {
    std::vector<Type*> arg_types{Type::getInt32Ty(context), Type::getInt8PtrTy(context)};
    auto return_type = Type::getInt8PtrTy(context);
    FunctionType *FT = FunctionType::get(return_type, arg_types, false);
    init_table_func = Function::Create(FT, Function::ExternalLinkage, "init_table", module);
  }
  return init_table_func;
}

Function* print_results_func = NULL;
Function* get_print_results_func(LLVMContext &context, Module* module) {
  if (print_results_func == NULL) {
    std::vector<Type*> arg_types{};
    auto return_type = Type::getVoidTy(context);
    FunctionType *FT = FunctionType::get(return_type, arg_types, false);
    print_results_func = Function::Create(FT, Function::ExternalLinkage, "print_results", module);
  }
  return print_results_func;
}

Function* inc_table_entry_func = NULL;
Function* get_inc_table_entry_func(LLVMContext &context, Module* module) {
  if (inc_table_entry_func == NULL) {
    std::vector<Type*> arg_types{Type::getInt8PtrTy(context), Type::getInt32Ty(context)};
    auto return_type = Type::getVoidTy(context);
    FunctionType *FT = FunctionType::get(return_type, arg_types, false);
    inc_table_entry_func = Function::Create(FT, Function::ExternalLinkage, "inc_table_entry", module);
  }
  return inc_table_entry_func;
}

void verify_cfg(Child_Parent_CFG cfg) {
  int num_entry = 0;
  int num_exit = 0;
  for (auto it = cfg.begin(); it != cfg.end(); it++) {
    auto parents = it->second.second;
    if (parents.size() == 0) {
      num_entry++;
    } 

    auto children = it->second.first;
    if (children.size() == 0) {
      num_exit++;
    }

  }
  if (num_entry != 1 && num_exit != 1) {
    throw "No unique entry and exit to cfg";
  }
}

void insert_path_counter(Function* F, BasicBlock* exit, Value* table_ptr, Value* r_ptr) {
  auto &context = F->getContext();
  Module* module = F->getParent();
  // insert right before term
  Instruction* term = exit->getTerminator();
  IRBuilder<> builder(term);
  auto inc_table_entry = get_inc_table_entry_func(context, module);
  std::vector<Value*> args;
  auto table = builder.CreateLoad(table_ptr);
  args.push_back(table);
  auto idx = builder.CreateLoad(r_ptr);
  args.push_back(idx);
  builder.CreateCall(inc_table_entry, args);
}

void insert_r_inc(Function* F, BasicBlock* from_block, BasicBlock* to_block, Value* r_ptr, int inc) {
  auto &context = F->getContext();
  Module* module = F->getParent();
  BasicBlock* block = BasicBlock::Create(context, "", F, to_block);
  IRBuilder<> builder(block);
  auto r = builder.CreateLoad(r_ptr);
  auto inc_const = ConstantInt::get(Type::getInt32Ty(context), inc, false);
  auto sum = builder.CreateAdd(r, inc_const);
  builder.CreateStore(sum, r_ptr);
  builder.CreateBr(to_block);

  auto term = from_block->getTerminator();
  for (int i = 0; i < term->getNumSuccessors(); i++) {
    if (term->getSuccessor(i) == to_block) {
      term->setSuccessor(i, block);
      break;
    }
  }
}

// return r_ptr and table_ptr
std::pair<Value*, Value*> insert_check(Function* F, BasicBlock* entry, int table_size) {
  auto table_var_name = "__" + F->getName() + "_table";
  Module* module = F->getParent();
  auto &context = F->getContext();
  auto ptr_type = PointerType::get(Type::getInt8Ty(context),0);
  auto null = ConstantPointerNull::get(ptr_type);
  GlobalVariable* table_var_ptr = new GlobalVariable(*module, 
          ptr_type,
          false, // is constant
          GlobalValue::PrivateLinkage,
          0, // has initializer, specified below
          table_var_name);
  table_var_ptr->setAlignment(8);
  auto zero = ConstantInt::get(Type::getInt64Ty(context), 0, false);
  auto zero32 = ConstantInt::get(Type::getInt32Ty(context), 0, false);
  // Global Variable Definitions
  table_var_ptr->setInitializer(null);

  BasicBlock* check_block = BasicBlock::Create(context, "check_init", F, entry);
  BasicBlock* is_uninit = BasicBlock::Create(context, "is_uninit", F);

  IRBuilder<> check_builder(check_block);
  auto r_ptr = check_builder.CreateAlloca(Type::getInt32Ty(context));
  check_builder.CreateStore(zero32, r_ptr);
  auto table_var = check_builder.CreateLoad(ptr_type, table_var_ptr);
  auto ptr_diff = check_builder.CreatePtrDiff(table_var, null);
  auto is_zero = check_builder.CreateICmpEQ(ptr_diff, zero);
  check_builder.CreateCondBr(is_zero, is_uninit, entry);

  IRBuilder<> init_builder(is_uninit);

  auto init_table = get_init_table_func(context, module);
  std::vector<Value*> args;
  auto n = ConstantInt::get(Type::getInt32Ty(context), table_size, false);
  args.push_back(n);
  auto func_name = init_builder.CreateGlobalString(F->getName());
  auto func_name_ptr = init_builder.CreatePointerCast(func_name, ptr_type);
  args.push_back(func_name_ptr);
  auto table = init_builder.CreateCall(init_table, args);
  init_builder.CreateStore(table, table_var_ptr);

  init_builder.CreateBr(entry);

  std::pair<Value*, Value*> r_ptr_table_ptr(r_ptr, table_var_ptr);
  return r_ptr_table_ptr;
}

using Edge_set = std::vector<std::pair<BasicBlock*, BasicBlock*>>;

Edge_set generate_edges(Child_Parent_CFG cfg, Function* F) {
  Edge_set edges;
  for (auto it = cfg.begin(); it != cfg.end(); it++) {
    auto from_block = it->first;
    auto children = it->second.first;
    for (auto child_it = children.begin(); child_it != children.end(); child_it++) {
      auto to_block = *child_it;
      std::pair<BasicBlock*, BasicBlock*> edge(from_block, to_block);
      edges.push_back(edge);
    }
  }
  return edges;
}

std::vector<BasicBlock*> topological_sort(Child_Parent_CFG cfg, Edge_set edges) {
  std::unordered_map<BasicBlock*, int> num_incoming;
  for (auto it = edges.begin(); it != edges.end(); it++) {
    auto to_block = it->second;
    int num_edges = num_incoming[to_block];
    num_edges++;
    num_incoming[to_block] = num_edges;
  }

  std::vector<BasicBlock*> sort;
  for (auto it = cfg.begin(); it != cfg.end(); it++) {
    int num_edges = num_incoming[it->first];
    if (num_edges == 0) {
      sort.push_back(it->first);
    }
  }

  for (std::vector<BasicBlock*>::size_type i = 0; i < sort.size(); i++) {
    auto to_remove = sort[i];
    auto children = cfg[to_remove].first;
    for (auto it = children.begin(); it != children.end(); it++) {
      auto child = *it;
      int num_edges = num_incoming[child];
      num_edges--;
      num_incoming[child] = num_edges;
      if (num_edges == 0) {
        sort.push_back(child);
      }
    }
  }

  return sort;
}

using Vals = std::unordered_map<BasicBlock*, std::unordered_map<BasicBlock*, int>>;
Vals generate_vals(std::vector<BasicBlock*> sort, Child_Parent_CFG cfg) {
  Vals val;
  std::unordered_map<BasicBlock*, int> num_paths;
  for (auto rit = sort.rbegin(); rit != sort.rend(); rit++) {
    auto block = *rit;
    auto children = cfg[block].first;
    if (children.size() == 0) {
      // this is the exit node
      num_paths[block] = 1;
    } else {
      num_paths[block] = 0;
      std::unordered_map<BasicBlock*, int> to_blocks;
      for (auto it = children.begin(); it != children.end(); it++) {
        auto to_block = *it;
        to_blocks[to_block] = num_paths[block];
        num_paths[block] = num_paths[block] + num_paths[to_block];
      }
      val[block] = to_blocks;
    }
  }

  return val;
}

using Tree = std::unordered_map<BasicBlock*, std::unordered_set<BasicBlock*>>;
// currently each edge has cost function = 1, so max spanning tree is one with maximum edges
Tree gen_max_spanning_tree(Edge_set edges, BasicBlock* entry, BasicBlock* exit) {
  // for each edge (u,v) this tree contains (u -> S where v in S and v -> T where u in T)
  Tree tree;
  // insert entry to exit edge so that it is not a chord
  tree[entry].insert(exit);
  tree[exit].insert(entry);

  // should probably shuffle edges, but this is simpler
  for (auto &edge : edges) {
    auto u = edge.first;
    auto v = edge.second;
    // if at least one endpoint not in the tree, add the edge to the tree
    if (tree.find(u) == tree.end() || tree.find(v) == tree.end()) {
      tree[u].insert(v);
      tree[v].insert(u);
    }
  }

  return tree;
}

// find path from v to u through tree
int get_inc(BasicBlock* v, BasicBlock* u, Vals vals, Tree tree, std::unordered_set<BasicBlock*> visited) {
  // errs() << "visiting " << v->getName() << "\n";
  if (v == u) {
    return 0;
  }

  int inc = INT_MIN;
  visited.insert(v);
  for (auto &next : tree[v]) {
    // only visit unvisited nodes
    if (visited.find(next) == visited.end()) {
      int next_inc = get_inc(next, u, vals, tree, visited);
      if (next_inc != INT_MIN) {
        int val = vals[v][next];
        if (val == 0) {
          val = -vals[next][v];
        }
        inc = next_inc + val;
        break;
      }
    }
  }
  return inc;
}

// for chord e=(u,v) of max spanning tree, Inc(e) = directed sum of Val(e') for all e' on the
// path from v to u in the max spanning tree plus Val(e). I.e. if we take an edge e' in reverse
// from the directed cfg then we subtract Val(e') from Inc(e).
using Incs = std::unordered_map<BasicBlock*, std::unordered_map<BasicBlock*, int>>;
Incs generate_incs(Edge_set edges, Vals vals, Tree tree) {
  Incs incs;
  for (auto &edge : edges) {
    auto u = edge.first;
    auto v = edge.second;
    // check if edge is a chord of tree
    if (tree[u].find(v) == tree[u].end()) {
      // errs() << "find inc from " << u->getName() << " - " << v->getName() << "\n";
      std::unordered_set<BasicBlock*> visited;
      int inc = get_inc(v, u, vals, tree, visited);
      if (inc == INT_MIN) {
        throw "Unable to compute inc";
      }
      inc += vals[u][v];
      auto u_map = incs[u];
      u_map[v] = inc;
      incs[u] = u_map;
    }
  }

  return incs;
}

void generate_all_edge_counters(Incs incs, Function* F, Value* r_ptr) {
  for (auto it = incs.begin(); it != incs.end(); it++) {
    auto from_block = it->first;
    auto to_blocks = it->second;
    for (auto eit = to_blocks.begin(); eit != to_blocks.end(); eit++) {
      auto to_block = eit->first;
      int inc = eit->second;
      if (inc != 0) {
        insert_r_inc(F, from_block, to_block, r_ptr, inc);
      }
    }
  }
}

std::string block = "block";

namespace {
  struct SkeletonPass : public FunctionPass {
    static char ID;
    SkeletonPass() : FunctionPass(ID) {}

    virtual bool runOnFunction(Function &F) {
      errs() << "Function: " << F.getName() << "\n";

      auto cfg = generate_child_parent_cfg(F);
      verify_cfg(cfg);

      auto entry = get_entry(cfg);
      auto exit = get_exit(cfg);

      int block_num = 0;
      for (auto &B : F) {
        B.setName(block + std::to_string(block_num++));
      }

      // auto block = insert_block(&F, entry, 3);

      auto edges = generate_edges(cfg, &F);

      auto sort = topological_sort(cfg, edges);
      errs() << "sort:\n";
      for (auto block : sort) {
        errs() << block->getName() << "\n";
      }

      auto vals = generate_vals(sort, cfg);
      vals[exit][entry] = 0;

      errs() << "vals:\n";
      for (auto sit = vals.begin(); sit != vals.end(); sit++) {
        auto from_block = sit->first;
        auto to_blocks = sit->second;
        for (auto eit = to_blocks.begin(); eit != to_blocks.end(); eit++) {
          auto to_block = eit->first;
          int val = eit->second;
          errs() << from_block->getName() << " - " << to_block->getName() << " : " << val << "\n";
        }
      }

      auto tree = gen_max_spanning_tree(edges, entry, exit);

      errs() << "max spanning tree edges:\n";
      for (auto sit = tree.begin(); sit != tree.end(); sit++) {
        auto from_block = sit->first;
        auto to_blocks = sit->second;
        for (auto &to_block : to_blocks) {
          if ((from_block == exit && to_block == entry) || vals[from_block].find(to_block) != vals[from_block].end()) {
            errs() << from_block->getName() << " - " << to_block->getName() << "\n";
          }
        }
      }

      auto incs = generate_incs(edges, vals, tree);
      errs() << "incs:\n";
      int num_incs = 0;
      for (auto sit = incs.begin(); sit != incs.end(); sit++) {
        auto from_block = sit->first;
        auto to_blocks = sit->second;
        for (auto eit = to_blocks.begin(); eit != to_blocks.end(); eit++) {
          auto to_block = eit->first;
          int inc = eit->second;
          num_incs++;
          errs() << from_block->getName() << " - " << to_block->getName() << " : " << inc << "\n";
        }
      }

      auto r_ptr_table_ptr = insert_check(&F, entry, num_incs);
      auto r_ptr = r_ptr_table_ptr.first;
      auto table_ptr = r_ptr_table_ptr.second;

      generate_all_edge_counters(incs, &F, r_ptr);

      insert_path_counter(&F, exit, table_ptr, r_ptr);

      if (F.getName() == "main") {
        Module* module = F.getParent();
        auto &context = F.getContext();
        for (auto &B : F) {
          Instruction* t = B.getTerminator();
          if (auto *op = dyn_cast<ReturnInst>(t)) {
            IRBuilder<> builder(op);
            auto print_results = get_print_results_func(context, module);
            std::vector<Value*> args;
            builder.CreateCall(print_results, args);
          }
        }
      }

      return false;
    }
  };
}

char SkeletonPass::ID = 0;

RegisterPass<SkeletonPass> X("skeleton", "simple pass"); 
