#ifndef LLVM_CODEGEN_ACCESSFREQUENCY_H
#define LLVM_CODEGEN_ACCESSFREQUENCY_H

#include "llvm/Value.h"
#include "llvm/CodeGen/MachineBasicBlock.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineOperand.h"
#include "llvm/Analysis/SPM.h"
#include "llvm/CodeGen/MachineLoopInfo.h"

namespace llvm{
	
	class StaticProfilePass;
class AccessFrequency : public MachineFunctionPass
{
	static AccessFrequency* ms_instance;
	
public:
	static AccessFrequency *Instance();
	static void Release();
	
public:
		//typedef MachineMemOperand **mmo_iterator;
        static char ID; // Pass identification, replacement for typeid
        AccessFrequency() : MachineFunctionPass(ID) {} //ctor;
        virtual ~AccessFrequency() {};
/*        unsigned int GetRead(int nReg)
        {
            DenseMap<int , unsigned>::iterator m_i = m_ReadMap.find(nReg);
            if( m_i == m_ReadMap.end())
                return 0;
            else
                return m_ReadMap[nReg];
        }

        unsigned GetWrite(int nReg)
        {
            DenseMap<int, unsigned>::iterator m_i = m_WriteMap.find(nReg);
            if( m_i == m_WriteMap.end())
                return 0;
            else
                return m_WriteMap[nReg];
        }
*/
public:
        void getAnalysisUsage(AnalysisUsage &AU) const;
        virtual bool runOnMachineFunction(MachineFunction &MF);
        void print(raw_ostream &OS) const;
		void print(raw_ostream &OS, const Module *) const {};		
        void dump();
		void reset();
		void initialize( const MachineLoopInfo * li, const StaticProfilePass *sp ) { MLI = li;  SP = sp;}

    protected:
    public:    
		/*DenseMap<int, CData *> m_hRegs;*/
        DenseMap<int, double> m_RegReadMap;
        DenseMap<int, double> m_RegWriteMap;
		StringMap<double> m_StackReadMap;
		StringMap<double> m_StackWriteMap;

    private:   // Intermediate data structures
        MachineFunction *MF;

        MachineRegisterInfo* MRI;

        const TargetRegisterInfo *TRI;
		
		const MachineLoopInfo *MLI;
		const StaticProfilePass *SP;
};
}  // End llvm namespace

#endif // LLVM_CODEGEN_ACCESSFREQUENCY_H