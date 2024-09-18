#include "llvm/Pass.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Instructions.h"
#include <vector>
#include <algorithm>
#include <stack>

using namespace llvm;

namespace {

struct DeadCodeElimination : public PassInfoMixin<DeadCodeElimination> {
    PreservedAnalyses run(Module &M, ModuleAnalysisManager &AM) {
    	std::vector<Value *> used;
		std::vector<std::vector<Value *>> usedInsideBB;
    	std::stack<Instruction *> removeListInstr;	

    	for(auto &F : M)
		{
				errs() << "In function: " << F.getName() << "\n\n";
				bool flagIterate = true;

				while(flagIterate)
				{
					flagIterate = false;

					// Loop and collect all instruction arguments in each basic block
					for(auto &B : F) 
					{
						for(auto &I : B) 
						{
							if(!dyn_cast<StoreInst>(&I)) 
							{
								for(int i = 0; i < I.getNumOperands(); i++)
								{
									if(!dyn_cast<ConstantInt>(I.getOperand(i))) 
									{
										used.push_back(I.getOperand(i));
									}
								}
							}
							else
							{
								// Constant stores are ignored, but stores that store fromon location to another cannot be ignored
								if(!dyn_cast<ConstantInt>(I.getOperand(0))) 
								{
									used.push_back(I.getOperand(0));
								}
							}
						}					
					}
					
					// Loop over each instruction in the block again, but check destination field
					// If it is on the list, remove it from it
					for(auto &B : F) 
					{
						for(auto &I : B) 
						{
							// Terminators should not be touched (eg return statements)
							
							if(!I.isTerminator() && !dyn_cast<CallInst>(&I))
							{
								if(dyn_cast<StoreInst>(&I)) 
								{
									if(std::find(used.begin(), used.end(), I.getOperand(1)) == used.end())
									{
										errs() << I << " can be deleted\n";
										removeListInstr.push(&I);
										flagIterate = true;
									}
								}
								else
								{
									if(std::find(used.begin(), used.end(), &I) == used.end())
									{
										errs() << I << " can be deleted\n";
										removeListInstr.push(&I);
										flagIterate = true;
									}
								}
							}
						}
					}

					errs() << "\n";

					// Remove all instructions on the stack

					while(!removeListInstr.empty())
					{
						Instruction *I=removeListInstr.top();
						removeListInstr.pop();
						I->eraseFromParent();
					}

					used.clear();
				}
			
		}
		
		return PreservedAnalyses::none();
    }
};

}

extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {
    return {
        .APIVersion = LLVM_PLUGIN_API_VERSION,
        .PluginName = "dead-code-elimination",
        .PluginVersion = "v0.1",
        .RegisterPassBuilderCallbacks = [](PassBuilder &PB) {
            PB.registerPipelineStartEPCallback(
                [](ModulePassManager &MPM, OptimizationLevel Level) {
                    MPM.addPass(DeadCodeElimination());
                });
        }
    };
}
