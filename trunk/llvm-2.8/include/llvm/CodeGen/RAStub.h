#ifndef RASTUB_H
#define RASTUB_H

#define DEBUG_TYPE "regalloc"
#include "llvm/BasicBlock.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/CodeGen/Passes.h"
#include "llvm/Target/TargetInstrInfo.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/IndexedMap.h"
#include "llvm/ADT/SmallSet.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/ADT/STLExtras.h"
#include <algorithm>
#include "PHIElimination.h"
#include "llvm/PassAnalysisSupport.h"
#include "llvm/CodeGen/AccessFrequency.h"

using namespace llvm;
class RAStub: public MachineFunctionPass
{
    public:
        RAStub() : MachineFunctionPass(ID) {};
        virtual ~RAStub();

    public:
        bool runOnMachineFunction(MachineFunction &);
		virtual void getAnalysisUsage(AnalysisUsage &AU) const 
		{
			  AU.setPreservesCFG();
			  AU.addRequiredID(PHIEliminationID);
			  AU.addRequiredID(TwoAddressInstructionPassID);
			  AU.addRequiredID(AccessFrequency::ID);
			  MachineFunctionPass::getAnalysisUsage(AU);
		}

    protected:
    private:
        static char ID;
        const TargetMachine *TM;
        MachineFunction *MF;
        const TargetRegisterInfo *TRI;
};

#endif // RASTUB_H
