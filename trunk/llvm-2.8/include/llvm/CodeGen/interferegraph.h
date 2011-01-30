#ifndef LLVM_CODEGEN_INTERFEREGRAPH_H
#define LLVM_CODEGEN_INTERFEREGRAPH_H

#include <map>
#include "MachineFunctionPass.h" // Base class: llvm::MachineFunctionPass
#include "llvm/Value.h"
#include "llvm/CodeGen/MachineBasicBlock.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineOperand.h"
#include "llvm/CodeGen/LiveIntervalAnalysis.h"

using namespace std;

namespace llvm
{

class InterfereGraph : public llvm::MachineFunctionPass {

	static InterfereGraph* ms_instance;

public:
        void getAnalysisUsage(AnalysisUsage &AU) const {};
        virtual bool runOnMachineFunction(MachineFunction &MF);
		inline void initialize(LiveIntervals *lis, const TargetRegisterInfo *tri) { li_ = lis; tri_ = tri;}
        void print(raw_ostream &OS) const ;
		void print(raw_ostream &OS, const Module *) const {};
		void printIG(raw_ostream &OS);
        void dump();		
		
public:
	static char ID;
	/*, li_ (NULL), tri_ (NULL)*/
	InterfereGraph(): MachineFunctionPass(ID){}
	virtual ~InterfereGraph() {};
	static InterfereGraph* Instance();
	static void Release();

public:
	LiveIntervals *li_;
	const TargetRegisterInfo *tri_;
	map<int, set<int> > m_IGraph;
	MachineFunction *MF;



};

}

#endif // LLVM_CODEGEN_INTERFEREGRAPH_H
