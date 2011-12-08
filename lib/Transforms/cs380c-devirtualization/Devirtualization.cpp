/*
 * Devirtualization.cpp
 *
 *  Created on: Nov 16, 2011
 *      Author: vitor, brian
 */

#include "llvm/Function.h"
#include "llvm/Module.h"
#include "llvm/Pass.h"
#include "llvm/Instructions.h"
#include "llvm/LLVMContext.h"

#include "llvm/Metadata.h"

#include "llvm/DerivedTypes.h"
#include "llvm/InstrTypes.h"

#include "llvm/Support/FormattedStream.h"
#include "llvm/Analysis/DebugInfo.h"

#include <string>

using namespace llvm;

namespace {
class DevirtualizationPass : public llvm::ModulePass {
public:
	static char ID;

	DevirtualizationPass(void) : ModulePass(ID) {}

  DICompositeType TestClass;
  size_t TestVTableIndex;
	virtual bool runOnModule(Module& m) {
    TestVTableIndex = -1;
    const NamedMDNode* const sp = m.getNamedMetadata(Twine("llvm.dbg.sp"));
    if (!sp) {
      ferrs() << "No llvm.dbg.sp metadata found\n";
    }
    for (size_t i=0; i < sp->getNumOperands(); ++i) {
			if (const MDNode* const mdnode = dyn_cast<MDNode>(sp->getOperand(i))) {
        const DISubprogram subprogram = DISubprogram(mdnode);
        const DICompositeType type = subprogram.getContainingType();
        if (type.Verify() && type.getTag() == dwarf::DW_TAG_class_type) {
          type->dump();
          DIArray arr = type.getTypeArray();
          for (size_t a=0; a < arr.getNumElements(); ++a) {
            DIDescriptor elem = arr.getElement(a);
            switch (elem.getTag()) {
              case dwarf::DW_TAG_member: {
                DIDerivedType member = DIDerivedType(&*elem);
                if (member.getName().str().substr(0,6).compare("_vptr$") == 0) {
                  TestClass = type;
                  TestVTableIndex = a;
                  elem->dump();
                  // map class to vtable
                }
                break;
              }
              case dwarf::DW_TAG_inheritance: {
                DIDerivedType inheritance = DIDerivedType(&*elem);
                // may have to go up hierarchy to look up vtable
                break;
              }
            }
          }
        }
      }
    }
		for (Module::iterator i = m.begin(), e = m.end(); i != e; ++i) {
			runOnFunction(*i);
		}
		return false;
	}

protected:
	void runOnFunction(Function& f) {
		for (Function::iterator i = f.begin(), e = f.end(); i != e; ++i) {
			runOnBasicBlock(*i);
		}
	}

	void runOnBasicBlock(BasicBlock& bb) {
		for (BasicBlock::iterator i = bb.begin(), e = bb.end(); i != e; ++i) {
			if (const CallInst* const Call = dyn_cast<CallInst>(&*i)) {
        if (const MDNode* const VirtualMD = Call->getMetadata("virtual-call")) {
				  ferrs() << "Call \"";
				  Call->print(ferrs());
				  ferrs() << '\n';
          if (const Function* const Func =
              dyn_cast<Function>(VirtualMD->getOperand(0))) {
            //if (const MDNode* const FuncMD = Func->getMetadata("dbg")) {
            //  FuncMD->dump();
            //}
          }
        }
			}
		}
	}
};

char DevirtualizationPass::ID= 0;
static RegisterPass<DevirtualizationPass>X("devirt", "Devirtualize virtual function calls", false, false);
}