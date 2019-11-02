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

using namespace llvm;

using Child_Parent_CFG = std::unordered_map<BasicBlock*, std::pair<std::vector<BasicBlock*>, std::vector<BasicBlock*>>>;

Child_Parent_CFG generate_child_parent_cfg(Function &F) {
  std::unordered_map<BasicBlock*, std::vector<BasicBlock*>> cfg_child_ptrs;
  std::unordered_map<BasicBlock*, std::vector<BasicBlock*>> cfg_parent_ptrs;

  for (auto it = F.begin(); it != F.end(); it++) {
    // errs() << "A block\n";
    auto B = &(*it);
    const Instruction* t = B->getTerminator();
    int i;
    int numSuc = t->getNumSuccessors();
    std::vector<BasicBlock*> succs;
    for (i = 0; i < numSuc; i++) {
      BasicBlock* succ = t->getSuccessor(i);
      // errs() << "Succ: " << *succ << "\n";

      auto entry = cfg_parent_ptrs.find(succ);
      if (entry == cfg_parent_ptrs.end()) {
        std::vector<BasicBlock*> parents;
        parents.push_back(B);
        std::pair<BasicBlock*, std::vector<BasicBlock*>> parent(succ, parents);
        cfg_parent_ptrs.insert(parent);
      } else {
        entry->second.push_back(B);
      }
      succs.push_back(succ);
    }
    std::pair<BasicBlock*, std::vector<BasicBlock*>> children(B, succs);
    cfg_child_ptrs.insert(children);
  }

  Child_Parent_CFG cfg;

  for (auto it = cfg_child_ptrs.begin(); it != cfg_child_ptrs.end(); it++) {
    auto entry = cfg_parent_ptrs.find(it->first);
    std::pair<std::vector<BasicBlock*>, std::vector<BasicBlock*>> child_parent;
    if (entry == cfg_parent_ptrs.end()) {
      std::vector<BasicBlock*> empty;
      child_parent = std::pair<std::vector<BasicBlock*>, std::vector<BasicBlock*>>(it->second, empty);
      
    } else {
      child_parent = std::pair<std::vector<BasicBlock*>, std::vector<BasicBlock*>>(it->second, entry->second);
    }
    std::pair<BasicBlock*, std::pair<std::vector<BasicBlock*>, std::vector<BasicBlock*>>> cfg_entry (it->first, child_parent);
    cfg.insert(cfg_entry);

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

// Function* static_print_func = NULL;
// Function* getPrintFunct(LLVMContext &context, Module* module) {
//   if (static_print_func == NULL) {
//     std::vector<Type*> arg_types{Type::getInt32Ty(context)};
//     auto return_type = Type::getVoidTy(context);
//     FunctionType *FT = FunctionType::get(return_type, arg_types, false);
//     static_print_func = Function::Create(FT, Function::ExternalLinkage, "print_num", module);
//   }
//   return static_print_func;
// }

Function* init_edge_table_func = NULL;
Function* get_init_edge_table_func(LLVMContext &context, Module* module) {
  if (init_edge_table_func == NULL) {
    std::vector<Type*> arg_types{Type::getInt32Ty(context), Type::getInt8PtrTy(context)};
    auto return_type = Type::getInt8PtrTy(context);
    FunctionType *FT = FunctionType::get(return_type, arg_types, false);
    init_edge_table_func = Function::Create(FT, Function::ExternalLinkage, "init_edge_table", module);
  }
  return init_edge_table_func;
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

void insert_edge_counter(Function* F, BasicBlock* from_block, BasicBlock* to_block, Value* table, int edge_idx) {
  auto &context = F->getContext();
  Module* module = F->getParent();
  BasicBlock* block = BasicBlock::Create(context, "", F, to_block);
  IRBuilder<> builder(block);
  auto inc_table_entry = get_inc_table_entry_func(context, module);
  std::vector<Value*> args;
  args.push_back(table);
  auto idx = ConstantInt::get(Type::getInt32Ty(context), edge_idx, false);
  args.push_back(idx);
  builder.CreateCall(inc_table_entry, args);
  builder.CreateBr(to_block);

  auto term = from_block->getTerminator();
  for (int i = 0; i < term->getNumSuccessors(); i++) {
    if (term->getSuccessor(i) == to_block) {
      term->setSuccessor(i, block);
      break;
    }
  }
}

// int nums = 4;
// long long block_num = 0;
// std::string block("block");

void insert_check(Function* F, BasicBlock* entry, int num) {
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
  // Global Variable Definitions
  table_var_ptr->setInitializer(null);

  BasicBlock* check_block = BasicBlock::Create(context, "check_init", F, entry);
  BasicBlock* is_uninit = BasicBlock::Create(context, "is_uninit", F);

  IRBuilder<> check_builder(check_block);
  auto table_var = check_builder.CreateLoad(ptr_type, table_var_ptr);
  auto ptr_diff = check_builder.CreatePtrDiff(table_var, null);
  auto is_zero = check_builder.CreateICmpEQ(ptr_diff, zero);
  check_builder.CreateCondBr(is_zero, is_uninit, entry);

  IRBuilder<> init_builder(is_uninit);

  auto init_edge_table = get_init_edge_table_func(context, module);
  std::vector<Value*> args;
  auto n = ConstantInt::get(Type::getInt32Ty(context), num, false);
  args.push_back(n);
  auto func_name = init_builder.CreateGlobalString(F->getName());
  auto func_name_ptr = init_builder.CreatePointerCast(func_name, ptr_type);
  args.push_back(func_name_ptr);
  auto table = init_builder.CreateCall(init_edge_table, args);
  init_builder.CreateStore(table, table_var_ptr);

  init_builder.CreateBr(entry);
}

namespace {
  struct SkeletonPass : public FunctionPass {
    static char ID;
    SkeletonPass() : FunctionPass(ID) {}

    virtual bool runOnFunction(Function &F) {
      errs() << "A function " << F.getName() << "\n";

      auto cfg = generate_child_parent_cfg(F);
      auto entry = get_entry(cfg);

      // auto block = insert_block(&F, entry, 3);

      insert_check(&F, entry, 5);

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

      // hijack edge
      // auto entry_data = cfg.find(entry);
      // auto entry_succs = entry_data->second.first;
      // auto term = entry->getTerminator();
      // for (auto it = entry_succs.begin(); it < entry_succs.end(); it++) {
      //   auto block = insert_block(&F, *it, nums++);
      //   for (int i = 0; i < term->getNumSuccessors(); i++) {
      //     if (term->getSuccessor(i) == *it) {
      //       errs() << "Hijacked edge: " << nums - 1 << "\n";
      //       errs() << "From " << *term << " to " << *((*it)->getFirstNonPHI()) << "\n";
      //       term->setSuccessor(i, block);
      //       break;
      //     }
      //   }
      // }

      // errs() << "Entry: " << *entry << "\n";
      for (auto &B : F) {
        // errs() << "Saw block: " << block_num << "\n";
        // errs() << B << "\n";
        // B.setName(block + std::to_string(block_num++));
        // for (auto &I : B) {
          // if (auto *op = dyn_cast<BinaryOperator>(&I)) {
          //   // Insert at the point where the instruction `op` appears.
          //   IRBuilder<> builder(op);

          //   // Make a multiply with the same operands as `op`.
          //   Value *lhs = op->getOperand(0);
          //   Value *rhs = op->getOperand(1);
          //   Value *mul = builder.CreateMul(lhs, rhs);

          //   // Everywhere the old instruction was used as an operand, use our
          //   // new multiply instruction instead.
          //   for (auto &U : op->uses()) {
          //     User *user = U.getUser();  // A User is anything with operands.
          //     user->setOperand(U.getOperandNo(), mul);
          //   }

          //   // We modified the code.
          //   return true;
          // }
        // }
      }

      return false;
    }
  };
}

char SkeletonPass::ID = 0;

RegisterPass<SkeletonPass> X("skeleton", "simple pass"); 
