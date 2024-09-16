#include "llvm/Pass.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include <cmath>
#include <stack>

using namespace llvm;

namespace {

struct StrengthReduction : public PassInfoMixin<StrengthReduction> {
    PreservedAnalyses run(Module &M, ModuleAnalysisManager &AM) {
    std::stack<Instruction *> removeListInstr;
    
		for(auto &F : M)
		{
			for (auto& B : F) 
			{
				for(auto& I : B) 
				{
					if(auto* op = dyn_cast<BinaryOperator>(&I)) 
					{  
						// Check if operator is mul
						if(auto* opc = dyn_cast<MulOperator>(op))
						{  	
							// Get operands
							Value* lhs = op->getOperand(0);
							Value* rhs = op->getOperand(1);
							
							// Check if right operand is an integer constant divideable by 2
							if(ConstantInt* CI = dyn_cast<ConstantInt>(rhs)) 
							{
						  		if(CI->getBitWidth() <= 32) 
						  		{
									int constIntValue = CI->getSExtValue();
									
									if(constIntValue % 2 == 0)
									{
										int shiftAmount = std::log2(constIntValue);
										errs() << "Shift amount: " << shiftAmount << "\n";
										
										// Replace instrucion "mul" with "shl"
										IRBuilder<> builder(op);
										Value* shl = builder.CreateShl(lhs, ConstantInt::get(cast<Type>(Type::getInt32Ty(F.getContext())), APInt(32, shiftAmount)));
										
										for (auto& U : op->uses()) 
										{
											User* user = U.getUser();  
											user->setOperand(U.getOperandNo(), shl);
										}
										
										// Push replaced instruction onto the stack, later will be erased from IR
										removeListInstr.push(&I);
									}
						  		}
							}
							
							

							// Check if left operand is an integer constant divideable by 2
							if(ConstantInt* CI = dyn_cast<ConstantInt>(lhs)) 
							{
						  		if(CI->getBitWidth() <= 32) 
						  		{
									int constIntValue = CI->getSExtValue();
									
									if(constIntValue % 2 == 0)
									{
										int shiftAmount = std::log2(constIntValue);
										errs() << "Shift amount: " << shiftAmount << "\n";
										
										// Replace instrucion "mul" with "shl" 
										IRBuilder<> builder(op);
										// Replace oreder of operands, shoft amount should be right-operand
										Value* shl = builder.CreateShl(rhs, ConstantInt::get(cast<Type>(Type::getInt32Ty(F.getContext())), APInt(32, shiftAmount)));
										
										for (auto& U : op->uses()) 
										{
											User* user = U.getUser();  
											user->setOperand(U.getOperandNo(), shl);
										}
										
										// Push replaced instruction onto the stack, later will be erased from IR
										removeListInstr.push(&I);
									}
						  		}
							}		  	
						}


					}
				}
			}
		}
		
		// Remove all repleaced instructions
		while(!removeListInstr.empty())
		{
			Instruction *I=removeListInstr.top();
			removeListInstr.pop();
			I->eraseFromParent();
		}
		
		return PreservedAnalyses::none();
    }
};

}

extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {
    return {
        .APIVersion = LLVM_PLUGIN_API_VERSION,
        .PluginName = "StrengthReductionOpt",
        .PluginVersion = "v0.1",
        .RegisterPassBuilderCallbacks = [](PassBuilder &PB) {
            PB.registerPipelineStartEPCallback(
                [](ModulePassManager &MPM, OptimizationLevel Level) {
                    MPM.addPass(StrengthReduction());
                });
        }
    };
}
