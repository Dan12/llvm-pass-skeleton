#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "llvm/IR/Constants.h"
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

Function* static_print_func = NULL;
Function* getPrintFunct(LLVMContext &context, Module* module) {
  if (static_print_func == NULL) {
    std::vector<Type*> arg_types{Type::getInt32Ty(context)};
    auto return_type = Type::getVoidTy(context);
    FunctionType *FT = FunctionType::get(return_type, arg_types, false);
    static_print_func = Function::Create(FT, Function::ExternalLinkage, "print_num", module);
  }
  return static_print_func;
}

BasicBlock* insert_block(Function* F, BasicBlock* to_block, int num) {
  auto &context = F->getContext();
  Module* module = F->getParent();
  BasicBlock* block = BasicBlock::Create(context, "test_block", F);
  IRBuilder<> builder(block);
  auto Print = getPrintFunct(context, module);
  std::vector<Value*> args;
  auto n = ConstantInt::get(Type::getInt32Ty(context), num, false);
  args.push_back(n);
  builder.CreateCall(Print, args);
  builder.CreateBr(to_block);
  return block;
}

int nums = 4;

namespace {
  struct SkeletonPass : public FunctionPass {
    static char ID;
    SkeletonPass() : FunctionPass(ID) {}

    virtual bool runOnFunction(Function &F) {
      errs() << "A function " << F.getName() << "\n";

      auto cfg = generate_child_parent_cfg(F);
      auto entry = get_entry(cfg);

      // hijack edge
      auto entry_data = cfg.find(entry);
      auto entry_succs = entry_data->second.first;
      auto term = entry->getTerminator();
      for (auto it = entry_succs.begin(); it < entry_succs.end(); it++) {
        auto block = insert_block(&F, *it, nums++);
        for (int i = 0; i < term->getNumSuccessors(); i++) {
          if (term->getSuccessor(i) == *it) {
            errs() << "Hijacked edge: " << nums - 1 << "\n";
            errs() << "From " << *term << " to " << *((*it)->getFirstNonPHI()) << "\n";
            term->setSuccessor(i, block);
            break;
          }
        }
      }

      errs() << "Entry: " << *entry << "\n";
      for (auto &B : F) {
        for (auto &I : B) {
          if (auto *op = dyn_cast<BinaryOperator>(&I)) {
            // Insert at the point where the instruction `op` appears.
            IRBuilder<> builder(op);

            // Make a multiply with the same operands as `op`.
            Value *lhs = op->getOperand(0);
            Value *rhs = op->getOperand(1);
            Value *mul = builder.CreateMul(lhs, rhs);

            // Everywhere the old instruction was used as an operand, use our
            // new multiply instruction instead.
            for (auto &U : op->uses()) {
              User *user = U.getUser();  // A User is anything with operands.
              user->setOperand(U.getOperandNo(), mul);
            }

            // We modified the code.
            return true;
          }
        }
      }

      return false;
    }
  };
}

char SkeletonPass::ID = 0;

RegisterPass<SkeletonPass> X("skeleton", "simple pass"); 
