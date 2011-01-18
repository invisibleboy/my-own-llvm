#ifndef LLVM_CODEGEN_ACCESSFREQUENCY_H
#define LLVM_CODEGEN_ACCESSFREQUENCY_H

#include "llvm/Value.h"
#include "llvm/CodeGen/MachineBasicBlock.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineOperand.h"

namespace llvm{
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
        virtual ~AccessFrequency();
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



    protected:
    private:    
        DenseMap<int, unsigned> m_RegReadMap;
        DenseMap<int, unsigned> m_RegWriteMap;
		StringMap<unsigned> m_StackReadMap;
		StringMap<unsigned> m_StackWriteMap;

    private:   // Intermediate data structures
        MachineFunction *MF;

        MachineRegisterInfo* MRI;

        const TargetRegisterInfo *TRI;
};
}  // End llvm namespace

#endif // LLVM_CODEGEN_ACCESSFREQUENCY_H
