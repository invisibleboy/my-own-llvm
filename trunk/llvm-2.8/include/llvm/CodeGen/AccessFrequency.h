#ifndef LLVM_CODEGEN_ACCESSFREQUENCY_H
#define LLVM_CODEGEN_ACCESSFREQUENCY_H

#include <list>
#include <set>
#include "llvm/Value.h"
#include "llvm/CodeGen/MachineBasicBlock.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineOperand.h"
#include "llvm/CodeGen/MachineLoopInfo.h"

using namespace std;
namespace llvm{
	
	class StaticProfilePass;
	
	struct TraceRecord
	{
		int m_nReg;
		int m_nFreq;
		bool m_bRead;
	public:
		TraceRecord(int nReg, int nFreq, bool bRead = true)
		{
			m_nReg = nReg;
			m_nFreq = nFreq;
			m_bRead = bRead;
		}
	};
	
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
                return 0;d.
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
		void print(raw_ostream &OS, const Module *) const { };		
		void printInt( raw_ostream &OS );
		void printRead( raw_ostream &OS );
		void printWrite( raw_ostream &OS );
		void printSize( raw_ostream &OS);
		void printVars( raw_ostream &OS);
        void dump();
		void reset();
		void initialize( const MachineLoopInfo * li, const StaticProfilePass *sp ) { MLI = li;  SP = sp;}
		
		unsigned int getRegSize( const int nReg ) const;
		
	private:
		unsigned int SymbolToAddr(unsigned int funcAddr, unsigned int varOffset);
		
		void FindTrash();

    protected:
    public:    
		/*DenseMap<int, CData *> m_hRegs;*/
        DenseMap<int, double> m_RegReadMap;
        DenseMap<int, double> m_RegWriteMap;
		StringMap<double> m_StackReadMap;
		StringMap<double> m_StackWriteMap;
		MachineFunction *MF;
		
		std::set<int> m_TrashSet;

    private:   // Intermediate data structures       
		std::list<TraceRecord> m_SimTrace;
        MachineRegisterInfo* MRI;

        const TargetRegisterInfo *TRI;
		
		const MachineLoopInfo *MLI;
		const StaticProfilePass *SP;
		int m_nVars;
};
}  // End llvm namespace

#endif // LLVM_CODEGEN_ACCESSFREQUENCY_H
