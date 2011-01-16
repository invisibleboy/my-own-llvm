#ifndef LLVM_CODEGEN_INTERFEREGRAPH_H
#define LLVM_CODEGEN_INTERFEREGRAPH_H

#include "MachineFunctionPass.h" // Base class: llvm::MachineFunctionPass
#include "llvm/Value.h"
#include "llvm/CodeGen/MachineBasicBlock.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineOperand.h"

namespace llvm
{

class InterfereGraph : public llvm::MachineFunctionPass {

	static InterfereGraph* ms_instance;

public:
        void getAnalysisUsage(AnalysisUsage &AU) const;
        virtual bool runOnMachineFunction(MachineFunction &MF);
        void print(raw_ostream &OS) const;
		void print(raw_ostream &OS, const Module *) const {};
        void dump();
		
public:
	static InterfereGraph* Instance();
	static void Release();

private:
	InterfereGraph();
	~InterfereGraph();

public:
	virtual bool runOnMachineFunction(MachineFunction& MF);
};

}

#endif // LLVM_CODEGEN_INTERFEREGRAPH_H
