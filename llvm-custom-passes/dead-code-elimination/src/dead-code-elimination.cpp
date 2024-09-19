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
#include <utility>

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


					while(!removeListInstr.empty())
					{
						Instruction *I=removeListInstr.top();
						removeListInstr.pop();
						I->eraseFromParent();
					}			
					


					/* Local dead stores elimination */

					std::vector<std::pair<Value *, Instruction *>> lastDef;
					std::vector<Value *> usedVars;

					for(auto &B : F)
					{
						usedVars.clear();
						lastDef.clear();

						// Loop the basic block and gather all stores and uses
						for(auto &I : B)
						{
							if(dyn_cast<StoreInst>(&I))
							{	
								// Collect all definitions in one list
								if(!dyn_cast<ConstantInt>(I.getOperand(1))) 
								{
									lastDef.push_back(std::make_pair(I.getOperand(1), &I));
								}
								else
								{
									// And all uses of variables in another list
									if(!dyn_cast<ConstantInt>(I.getOperand(0)))
										usedVars.push_back(I.getOperand(0));
								}
							}	
							else
							{
								for(int i = 0; i < I.getNumOperands(); i++)
								{
									if(!dyn_cast<ConstantInt>(I.getOperand(i)))
										usedVars.push_back(I.getOperand(i));
								}
							}
						}

						// Loop through definition list, and if any variable that has been defined in the basic block is NOT used
						// inside this block, it must NOT be removed (global analysis needed for this) - maybe the value defined is needed in some other BB

						for(auto &d : lastDef)
						{
							auto key = d.first;
								
							auto isUsed = std::find_if(usedVars.begin(), usedVars.end(), [key](const auto& p) 
							{ 
								return p == key;
							});

							if(isUsed == usedVars.end())
							{
								auto it = std::find(lastDef.begin(), lastDef.end(), d);
								if(it != lastDef.end())
									lastDef.erase(it);
							}
							else
							{
								errs() << "Pair: " << d.first << " and " << *(d.second) << "\n";
							}
						}


						// Now loop again and remove from the list of definitions last definition of every used variable

						for(auto &I : B)
						{
							if(dyn_cast<StoreInst>(&I)) continue;

							for(int i = 0; i < I.getNumOperands(); i++)
							{
								if(dyn_cast<ConstantInt>(I.getOperand(i))) continue;

								auto key = I.getOperand(i);
								
								auto rmDef = std::find_if(lastDef.rbegin(), lastDef.rend(), [key](const auto& p) 
								{ 
									return p.first == key;
								});

								//if(rmDef->second == &I) continue;

								if(rmDef != lastDef.rend())
								{
									errs() << "ERASING pair: " << rmDef->first << " and " << *(rmDef->second) << "\n";
									lastDef.erase(std::next(rmDef).base());
								}

								
							}
						}	


						// Loop once again and over the same basic block, check instruction destinations and if any of them is still on the list,
						// it is safe to delete store instruction pointing to from that place in the list
						for(auto &I : B)
						{
							auto key = &I;
							auto rmInst = std::find_if(lastDef.begin(), lastDef.end(), [key](const auto& p) 
							{ 
								return p.first == key;
							});

							if(rmInst != lastDef.end())
							{
								removeListInstr.push(rmInst->second);
								lastDef.erase(rmInst);

								// Find all other occurencies of this instruction in list and erase those entries
								while(true)
								{
									//auto key = &I;
									auto it = std::find_if(lastDef.begin(), lastDef.end(), [key](const auto& p) 
									{ 
										return p.second == key;
									});

									if(it != lastDef.end())
									{
										lastDef.erase(it);
									}
									else
										break;
								}

								flagIterate = true;
							}
						} 
					}

					// Remove all instructions on the stack

					while(!removeListInstr.empty())
					{
						Instruction *I=removeListInstr.top();
						removeListInstr.pop();
						I->eraseFromParent();
					}

					used.clear();
					lastDef.clear();
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
