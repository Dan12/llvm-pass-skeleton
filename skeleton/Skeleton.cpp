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

typedef struct Edge {
  BasicBlock* from;
  BasicBlock* to;
  struct Edge* back_edge_entry_mapping; // points to back edge that this edge replaced going from entry to the 'to' node, otherwise null
  struct Edge* back_edge_exit_mapping; // points to back edge that this edge replaced going from 'from' node to exit, otherwise null
} Edge;

Edge* create_edge(BasicBlock* from, BasicBlock* to) {
  Edge* edge = (Edge*)malloc(sizeof(Edge));
  edge->from = from;
  edge->to = to;
  edge->back_edge_entry_mapping = NULL;
  edge->back_edge_exit_mapping = NULL;
  return edge;
}

// everything is a vector because the graph could be a multigraph

// a graph is a vector of edges
using Graph = std::vector<Edge*>;

std::vector<BasicBlock*> get_children(BasicBlock* block, Graph &g) {
  std::vector<BasicBlock*> children;
  for (auto &edge : g) {
    if (edge->from == block) {
      children.push_back(edge->to);
    }
  }
  return children;
}

std::vector<BasicBlock*> get_parents(BasicBlock* block, Graph &g) {
  std::vector<BasicBlock*> parents;
  for (auto &edge : g) {
    if (edge->to == block) {
      parents.push_back(edge->from);
    }
  }
  return parents;
}

std::vector<Edge*> get_incoming_edges(BasicBlock* block, Graph &g) {
  std::vector<Edge*> incoming_edges;
  for (auto &edge : g) {
    if (edge->to == block) {
      incoming_edges.push_back(edge);
    }
  }
  return incoming_edges;
}

std::vector<Edge*> get_outgoing_edges(BasicBlock* block, Graph &g) {
  std::vector<Edge*> outgoing_edges;
  for (auto &edge : g) {
    if (edge->from == block) {
      outgoing_edges.push_back(edge);
    }
  }
  return outgoing_edges;
}

bool directed_edge_exists(BasicBlock* u, BasicBlock* v, Graph &g) {
  for (auto &edge : g) {
    if (edge->from == u && edge->to == v) {
      return true;
    }
  }
  return false;
}

bool edge_exists(BasicBlock* u, BasicBlock* v, Graph &g) {
  for (auto &edge : g) {
    if ((edge->from == u && edge->to == v) || (edge->from == v && edge->to == u)) {
      return true;
    }
  }
  return false;
}

using Vertices = std::vector<BasicBlock*>;

Vertices generate_vertices(Function &F) {
  Vertices v;
  for (auto it = F.begin(); it != F.end(); it++) {
    auto B = &(*it);
    v.push_back(B);
  }
  return v;
}

Graph generate_graph(Vertices &v) {
  Graph g;
  for (auto it = v.begin(); it != v.end(); it++) {
    auto from_block = *it;
    const Instruction* t = from_block->getTerminator();
    int i;
    int numSuc = t->getNumSuccessors();
    // create a list of B's children
    std::vector<BasicBlock*> succs;
    for (i = 0; i < numSuc; i++) {
      BasicBlock* to_block = t->getSuccessor(i);

      auto edge = create_edge(from_block, to_block);
      g.push_back(edge);
    }
  }

  return g;
}

BasicBlock* get_entry(Vertices &v, Graph &g) {
  for (auto it = v.begin(); it != v.end(); it++) {
    auto block = *it;
    auto parents = get_parents(block, g);
    if (parents.size() == 0) {
      return block;
    }
  }
  throw "No entry block for cfg";
}

BasicBlock* get_exit(Vertices &v, Graph &g) {
  for (auto it = v.begin(); it != v.end(); it++) {
    auto block = *it;
    auto children = get_children(block, g);
    if (children.size() == 0) {
      return block;
    }
  }
  throw "No exit block for cfg";
}

void verify_graph(Vertices &v, Graph &g) {
  int num_entry = 0;
  int num_exit = 0;
  for (auto it = v.begin(); it != v.end(); it++) {
    auto block = *it;
    auto parents = get_parents(block, g);
    if (parents.size() == 0) {
      num_entry++;
    } 

    auto children = get_children(block, g);
    if (children.size() == 0) {
      num_exit++;
    }

  }
  if (num_entry != 1 && num_exit != 1) {
    throw "No unique entry and exit to cfg";
  }
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

void insert_loop_path_counter(Function* F, BasicBlock* from_block, BasicBlock* to_block, Value* table_ptr, Value* r_ptr, int inc, int reset) {
  auto &context = F->getContext();
  Module* module = F->getParent();
  BasicBlock* block = BasicBlock::Create(context, "", F, to_block);
  IRBuilder<> builder(block);

  if (inc != 0) {
    auto r = builder.CreateLoad(r_ptr);
    auto inc_const = ConstantInt::get(Type::getInt32Ty(context), inc, false);
    auto sum = builder.CreateAdd(r, inc_const);
    builder.CreateStore(sum, r_ptr);
  }

  // counter[r]++
  auto inc_table_entry = get_inc_table_entry_func(context, module);
  std::vector<Value*> args;
  auto table = builder.CreateLoad(table_ptr);
  args.push_back(table);
  auto idx = builder.CreateLoad(r_ptr);
  args.push_back(idx);
  builder.CreateCall(inc_table_entry, args);

  // r = init_loop_value
  auto reset_const = ConstantInt::get(Type::getInt32Ty(context), reset, false);
  builder.CreateStore(reset_const, r_ptr);

  builder.CreateBr(to_block);

  auto term = from_block->getTerminator();
  for (int i = 0; i < term->getNumSuccessors(); i++) {
    if (term->getSuccessor(i) == to_block) {
      term->setSuccessor(i, block);
      break;
    }
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

  // r = 0
  auto r_ptr = check_builder.CreateAlloca(Type::getInt32Ty(context));
  check_builder.CreateStore(zero32, r_ptr);

  // if table_ptr == NULL then init else entry
  auto table_var = check_builder.CreateLoad(ptr_type, table_var_ptr);
  auto ptr_diff = check_builder.CreatePtrDiff(table_var, null);
  auto is_zero = check_builder.CreateICmpEQ(ptr_diff, zero);
  check_builder.CreateCondBr(is_zero, is_uninit, entry);

  IRBuilder<> init_builder(is_uninit);

  // table_ptr = init_table(table_size, func_name)
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

std::vector<BasicBlock*> topological_sort(Vertices &v, Graph &g) {
  std::unordered_map<BasicBlock*, int> num_incoming;
  for (auto it = v.begin(); it != v.end(); it++) {
    auto to_block = *it;
    auto incoming = get_incoming_edges(to_block, g);
    num_incoming[to_block] = incoming.size();
  }

  std::vector<BasicBlock*> sort;
  for (auto it = v.begin(); it != v.end(); it++) {
    auto block = *it;
    if (num_incoming[block] == 0) {
      sort.push_back(block);
    }
  }

  for (std::vector<BasicBlock*>::size_type i = 0; i < sort.size(); i++) {
    auto to_remove = sort[i];
    auto children = get_children(to_remove, g);
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

using Vals = std::unordered_map<Edge*, int>;
Vals generate_vals(std::vector<BasicBlock*> &sort, Vertices &v, Graph &g, BasicBlock* entry) {
  Vals val;
  std::unordered_map<BasicBlock*, int> num_paths;
  for (auto rit = sort.rbegin(); rit != sort.rend(); rit++) {
    auto block = *rit;
    auto outgoing_edges = get_outgoing_edges(block, g);
    if (outgoing_edges.size() == 0) {
      // this is the exit node
      num_paths[block] = 1;
    } else {
      num_paths[block] = 0;
      for (auto it = outgoing_edges.begin(); it != outgoing_edges.end(); it++) {
        auto edge = *it;
        val[edge] = num_paths[block];
        auto to_block = edge->to;
        num_paths[block] = num_paths[block] + num_paths[to_block];
      }
    }
  }

  return val;
}

using Tree = std::unordered_set<Edge*>;
// return true if path from u to v, else return false
bool exists_path(BasicBlock* u, BasicBlock* v, Tree &tree, std::unordered_set<BasicBlock*> &visited) {
  if (u == v) {
    return true;
  }

  visited.insert(u);
  for (auto &edge : tree) {
    BasicBlock* next = NULL;
    if (edge->to == u) {
      next = edge->from;
    } else if (edge->from == u) {
      next = edge->to;
    }

    if (next != NULL) {
      if (visited.find(next) == visited.end()) {
        if (exists_path(next, v, tree, visited)) {
          return true;
        }
      }
    }
  }

  return false;
}

// currently each edge has cost function = 1, so max spanning tree is one with maximum edges
Tree gen_max_spanning_tree(Graph &g) {
  // for each edge (u,v) this tree contains (u -> S where v in S and v -> T where u in T)
  Tree tree;

  // should probably shuffle edges, but this is simpler
  // also, exit to entry node should be first node in g so that it isn't a chord
  for (auto &edge : g) {
    // order of u and v don't matter because we are checking for a path in an undirected tree
    auto u = edge->from;
    auto v = edge->to;
    // if there is not already a path from u to v in the tree, add the edge
    std::unordered_set<BasicBlock*> visited;
    if (exists_path(u, v, tree, visited) == false) {
      tree.insert(edge);
    }
  }

  return tree;
}

// find path from v to u through tree
int get_inc(BasicBlock* v, BasicBlock* u, Vals &vals, Tree &tree, std::unordered_set<BasicBlock*> &visited) {
  // errs() << "visiting " << v->getName() << "\n";
  if (v == u) {
    return 0;
  }

  int inc = INT_MIN;
  visited.insert(v);
  for (auto &edge : tree) {
    BasicBlock* next = NULL;
    if (edge->to == v) {
      next = edge->from;
    } else if (edge->from == v) {
      next = edge->to;
    }
    if (next != NULL) {
      // only visit unvisited nodes
      if (visited.find(next) == visited.end()) {
        int next_inc = get_inc(next, u, vals, tree, visited);
        if (next_inc != INT_MIN) {
          int val = vals[edge];
          // check if we traversed the edge backwards
          if (edge->to == v) {
            val = -val;
          }
          inc = next_inc + val;
          break;
        }
      }
    }
  }
  return inc;
}

// for chord e=(u,v) of max spanning tree, Inc(e) = directed sum of Val(e') for all e' on the
// path from v to u in the max spanning tree plus Val(e). I.e. if we take an edge e' in reverse
// from the directed cfg then we subtract Val(e') from Inc(e).
using Incs = std::unordered_map<Edge*, int>;
Incs generate_incs(Graph &g, Vals &vals, Tree &tree) {
  Incs incs;
  for (auto &edge : g) {
    // check if edge is a chord of tree
    if (tree.find(edge) == tree.end()) {
      auto u = edge->from;
      auto v = edge->to;
      // errs() << "find inc from " << u->getName() << " - " << v->getName() << "\n";
      std::unordered_set<BasicBlock*> visited;
      int inc = get_inc(v, u, vals, tree, visited);
      if (inc == INT_MIN) {
        throw "Unable to compute inc for chord";
      }
      inc += vals[edge];
      incs[edge] = inc;
    }
  }

  return incs;
}

void generate_all_edge_counters(Incs &incs, Function* F, Value* r_ptr) {
  for (auto it = incs.begin(); it != incs.end(); it++) {
    auto edge = it->first;
    auto from_block = edge->from;
    auto to_block = edge->to;
    int inc = it->second;
    if (edge->back_edge_entry_mapping == NULL && edge->back_edge_exit_mapping == NULL) {
      if (inc != 0) {
        insert_r_inc(F, from_block, to_block, r_ptr, inc);
      }
    }
  }
}

void generate_all_loop_counters(std::vector<Edge*> back_edges, Incs &incs, Function* F, Value* table_ptr, Value* r_ptr) {
  for (auto &edge: back_edges) {
    auto from_block = edge->from;
    auto to_block = edge->to;
    int inc = 0;
    int reset = 0;
    for (auto it = incs.begin(); it != incs.end(); it++) {
      auto inc_edge = it->first;
      auto inc_inc = it->second;
      // entry to top represents reset value
      if (inc_edge->back_edge_entry_mapping == edge) {
        reset = inc_inc;
      }
      // tail to exit represents inc before count increment
      if (inc_edge->back_edge_exit_mapping == edge) {
        inc = inc_inc;
      }
    }
    insert_loop_path_counter(F, from_block, to_block, table_ptr, r_ptr, inc, reset);
  }
}

int dfs_path_incs(BasicBlock* u, Graph &g, Incs &incs, int inc, Twine on_path) {
  auto new_string = on_path + "," + u->getName();
  auto outgoing_edges = get_outgoing_edges(u, g);
  if (outgoing_edges.size() == 0) {
    errs() << new_string << " : " << inc << "\n";
    return 1;
  } else {
    int num_paths = 0;
    for (auto &edge : outgoing_edges) {
      int new_inc = inc + incs[edge];
      if (edge->back_edge_entry_mapping != NULL) {
        num_paths += dfs_path_incs(edge->to, g, incs, new_inc, "Loop head at -> ");
      } else if (edge->back_edge_exit_mapping != NULL) {
        num_paths += 1;
        errs() << new_string << " loop up to " << edge->back_edge_exit_mapping->to->getName() << " : " << inc << "\n";
      } else {
        num_paths += dfs_path_incs(edge->to, g, incs, new_inc, new_string);
      }
    }
    return num_paths;
  }
}

void dfs_find_back_edges(BasicBlock* u, Graph &g, std::unordered_map<BasicBlock*, int>& color, std::vector<Edge*> &back_edges) {
  // color = 0 is white, 1 = gray, 2 = black
  color[u] = 1;
  auto outgoing_edges = get_outgoing_edges(u, g);
  for (auto &edge : outgoing_edges) {
    auto to_block = edge->to;
    if (color[to_block] != 0) {
      if (color[to_block] == 1) {
        back_edges.push_back(edge);
      }
    } else {
      dfs_find_back_edges(to_block, g, color, back_edges);
    }
  }
  color[u] = 2;
}

std::string block = "block";

namespace {
  struct SkeletonPass : public FunctionPass {
    static char ID;
    SkeletonPass() : FunctionPass(ID) {}

    virtual bool runOnFunction(Function &F) {
      errs() << "Function: " << F.getName() << "\n";

      int block_num = 0;
      // assign name to every block
      for (auto &B : F) {
        B.setName(block + std::to_string(block_num++));
      }

      auto v = generate_vertices(F);
      auto g = generate_graph(v);
      verify_graph(v, g);

      auto entry = get_entry(v, g);
      auto exit = get_exit(v, g);

      errs() << "Entry: " << entry->getName() << " , Exit: " << exit->getName() << "\n";

      std::unordered_map<BasicBlock*, int> color;
      std::vector<Edge*> back_edges;
      dfs_find_back_edges(entry, g, color, back_edges);
      errs() << "back edges:\n";
      for (auto &edge : back_edges) {
        errs() << edge->from->getName() << " - " << edge->to->getName() << "\n";
      }

      Graph g_minus_back_plus_loop;

      for (auto &edge : g) {
        if (std::find(back_edges.begin(), back_edges.end(), edge) != back_edges.end()) {
          // if [edge] is a back edge, dont add it to [g_minus_back_plus_loop]
          // and instead create two new edges from entry to head and tail to exit
          Edge* entry_to_head = create_edge(entry, edge->to);
          entry_to_head->back_edge_entry_mapping = edge;
          g_minus_back_plus_loop.push_back(entry_to_head);

          Edge* tail_to_exit = create_edge(edge->from, exit);
          tail_to_exit->back_edge_exit_mapping = edge;
          g_minus_back_plus_loop.push_back(tail_to_exit);
        } else {
          g_minus_back_plus_loop.push_back(edge);
        }
      }

      errs() << "Graph:\n";
      for (auto &edge : g_minus_back_plus_loop) {
        errs() << edge->from->getName() << " - " << edge->to->getName() << "\n";
      }

      auto sort = topological_sort(v, g_minus_back_plus_loop);
      errs() << "sort:\n";
      for (auto &block : sort) {
        errs() << block->getName() << "\n";
      }

      auto vals = generate_vals(sort, v, g_minus_back_plus_loop, entry);

      errs() << "vals:\n";
      for (auto it = vals.begin(); it != vals.end(); it++) {
        auto edge = it->first;
        auto val = it->second;
        errs() << edge->from->getName() << " - " << edge->to->getName() << " : " << val << "\n";
      }

      // add exit to entry edge

      Graph g_minus_back_plus_loop_plus_ete;
      Edge* exit_to_entry = create_edge(exit, entry);
      vals[exit_to_entry] = 0;
      g_minus_back_plus_loop_plus_ete.push_back(exit_to_entry);
      for (auto &edge : g_minus_back_plus_loop) {
        g_minus_back_plus_loop_plus_ete.push_back(edge);
      }

      auto tree = gen_max_spanning_tree(g_minus_back_plus_loop_plus_ete);

      errs() << "max spanning tree edges:\n";
      for (auto &edge : tree) {
        errs() << edge->from->getName() << " - " << edge->to->getName() << "\n";
      }

      auto incs = generate_incs(g_minus_back_plus_loop_plus_ete, vals, tree);

      errs() << "incs:\n";
      for (auto it = incs.begin(); it != incs.end(); it++) {
        auto edge = it->first;
        auto inc = it->second;
        errs() << edge->from->getName() << " - " << edge->to->getName() << " : " << inc << "\n";
      }

      errs() << "Paths to sums:\n";
      int num_paths = dfs_path_incs(entry, g_minus_back_plus_loop, incs, 0, "");
      errs() << "Num paths: " << num_paths << "\n";

      // insert the table init check and the r=0 instruction
      auto r_ptr_table_ptr = insert_check(&F, entry, num_paths);
      auto r_ptr = r_ptr_table_ptr.first;
      auto table_ptr = r_ptr_table_ptr.second;

      // insert the index increments
      generate_all_edge_counters(incs, &F, r_ptr);

      // insert path incrementers at back edges
      generate_all_loop_counters(back_edges, incs, &F, table_ptr, r_ptr);

      // insert the path incrementer at the exit node
      insert_path_counter(&F, exit, table_ptr, r_ptr);

      // print the results right before returning from main
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
