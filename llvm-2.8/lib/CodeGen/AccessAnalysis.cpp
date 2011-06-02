
#include <list>
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/Debug.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineBasicBlock.h"
#include "llvm/CodeGen/MachineOperand.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Target/TargetRegisterInfo.h"
#include "llvm/Target/TargetFrameInfo.h"
#include "llvm/CodeGen/RegisterScavenging.h"
#include "llvm/CodeGen/AccessAnalysis.h"
#include "llvm/CodeGen/MachineMemOperand.h"
#include "llvm/CodeGen/PseudoSourceValue.h"

extern std::map<std::string, std::set<std::string> > g_hFuncCall;
extern const std::map<const Function *, std::map<const BasicBlock *, double> > *g_hF2B2Acc; 

static bool InstructionStoresToFI(const MachineInstr *MI, int FI) {
  for (MachineInstr::mmo_iterator o = MI->memoperands_begin(),
         oe = MI->memoperands_end(); o != oe; ++o) {
    if (!(*o)->isStore() || !(*o)->getValue())
      continue;
	const FixedStackPseudoSourceValue *Value = dyn_cast<const FixedStackPseudoSourceValue>((*o)->getValue());
    if (Value) 
	{
      if (Value->getFrameIndex() == FI)
        return true;
    }
  }
  return false;
}

std::map<const Function *, std::set<AccessRecord *, RecordCmp> > AccessCount; 		// For access frequency
std::map<Function *, map<int, int64_t> > StackLayout;		// For stack layout

//using namespace StackLayout;

	bool AccessAnalysis(llvm::MachineFunction &mf)
	{
		const llvm::Function *fn = mf.getFunction();
		/*std::string szMain = "main";
		if(fn->getName() != szMain && g_hFuncCall[szMain].find(fn->getName()) == g_hFuncCall[szMain].end() )
		{
			errs() << "--------qali:--------Skip function " << fn->getName() << " in AccessAnalysis !\n";
			return true;
		}*/
		
		AccessCount[mf.getFunction()].clear();
		//std::map<int, AccessRecord *, RecordCmp> &fAccessCount = AccessCount[mf.getFunction()];
		std::map<int, AccessRecord *> AccessSet;
		
		for (MachineFunction::const_iterator BBI = mf.begin(), FE = mf.end();
			BBI != FE; ++BBI)
		{
			// Get access frequence for block
			double dFactor = 0.0;
			const BasicBlock *bb = BBI->getBasicBlock();
			if( bb != NULL )
			{
				std::map<const Function *, std::map<const BasicBlock *, double> >::const_iterator f2b2acc_p, E = g_hF2B2Acc->end();
				if( (f2b2acc_p = g_hF2B2Acc->find(fn) ) != E )
				{
					std::map<const BasicBlock *, double>::const_iterator b2acc_p, EE = f2b2acc_p->second.end();
					if( (b2acc_p = f2b2acc_p->second.find(bb) ) != EE )
						dFactor = b2acc_p->second;
				}
				if( dFactor == 0.0)
					dFactor = 1.0;
			}
			
			// Get access frequncy for each operand
			for (MachineBasicBlock::const_iterator MI = BBI->begin(), BBE = BBI->end();
            MI != BBE; ++MI)
			{
				DEBUG(MI->print(dbgs(), NULL ));				
				for (unsigned i = 0, e = MI->getNumOperands(); i != e; ++ i)
				{
					const MachineOperand &MO = MI->getOperand(i);
					if ( MO.isFI() && InstructionStoresToFI(MI, MO.getIndex()) )
					{
						int FrameIndex = MI->getOperand(i).getIndex();
						AccessRecord *rec = NULL;
						//if( fAccessCount.find(FrameIndex) == fAccessCount.end())	
						if( AccessSet.find(FrameIndex) == AccessSet.end())
						{
							rec = new AccessRecord();
							rec->m_nID = FrameIndex;
							rec->m_dCount = dFactor;
							AccessSet[rec->m_nID] = rec;
						}
						else
						{
							//rec = fAccessCount[FrameIndex];
							rec = AccessSet[FrameIndex];
							rec->m_dCount += dFactor;
						}
							
					}
				}
			}		
		}
		
		// copy into access counter
		std::set<AccessRecord *, RecordCmp> &fAccessCount = AccessCount[mf.getFunction()];
		std::map<int, AccessRecord *>::iterator AI = AccessSet.begin(), AE = AccessSet.end();
		for( ; AI != AE; ++ AI)
		{
			fAccessCount.insert(AI->second );
		}
		return true;
	}
	
	bool PackStack(MachineFunction &mf, int64_t &Offset, RegScavenger *RS, int min, int max, SmallSet<int, 16> &LargeStackObjs)
	{
		const TargetFrameInfo &TFI = *mf.getTarget().getFrameInfo();
		bool StackGrowsDown = TFI.getStackGrowthDirection() == TargetFrameInfo::StackGrowsDown;
		MachineFrameInfo *MFI = mf.getFrameInfo();
		
		
		set<int> Objects; 
		// search the stack objects needing allocation
		 for (unsigned i = 0, e = MFI->getObjectIndexEnd(); i != e; ++i)
		 {
			if (MFI->isObjectPreAllocated(i) &&	MFI->getUseLocalStackAllocationBlock() )
			  continue;
			if (i >= min && i <= max)
			  continue;
			if (RS && (int)i == RS->getScavengingFrameIndex())
			  continue;
			if (MFI->isDeadObjectIndex(i))
			  continue;
			if (MFI->getStackProtectorIndex() == (int)i)
			  continue;
			// qali
			if (LargeStackObjs.count(i))
			  continue;
			Objects.insert(i);
		 }
		 
		 // allocate the stack objects sorted by writing counter
		 std::set<AccessRecord *, RecordCmp> &fAccessCount = AccessCount[mf.getFunction()];	
		 std::set<AccessRecord *, RecordCmp> ::iterator I = fAccessCount.begin(), E = fAccessCount.end();
		 std::set<int> sorted;
		 for(; I != E; ++ I)
		 {
			if( Objects.find((*I)->m_nID ) == Objects.end() )
				continue;
			if (StackGrowsDown)
				Offset += MFI->getObjectSize((*I)->m_nID);
			unsigned Align = MFI->getObjectAlignment((*I)->m_nID);
			Offset = (Offset + Align - 1)/ Align * Align;
			
			if (StackGrowsDown) 
			{
				DEBUG(dbgs() << "alloc FI(" << (*I)->m_nID << ") at SP[" << -Offset << "]\n");
				MFI->setObjectOffset((*I)->m_nID, -Offset); // Set the computed offset
			} 
			else
			{
				DEBUG(dbgs() << "alloc FI(" << (*I)->m_nID << ") at SP[" << Offset << "]\n");
				MFI->setObjectOffset((*I)->m_nID, Offset);
				Offset += MFI->getObjectSize((*I)->m_nID);
			}			
			sorted.insert((*I)->m_nID );
		 }		
		 
		 // alloate the stack objects unsorted
		 std::set<int>::iterator OI = Objects.begin(), OE = Objects.end();
		 for(; OI != OE; ++ OI)
		 {
			if( sorted.find(*OI) != sorted.end())
				continue;
			if (StackGrowsDown)
				Offset += MFI->getObjectSize(*OI);
			unsigned Align = MFI->getObjectAlignment(*OI);
			Offset = (Offset + Align - 1)/ Align * Align;
			
			if (StackGrowsDown) 
			{
				DEBUG(dbgs() << "alloc FI(" << *OI << ") at SP[" << -Offset << "]\n");
				MFI->setObjectOffset(*OI, -Offset); // Set the computed offset
			} 
			else
			{
				DEBUG(dbgs() << "alloc FI(" << *OI << ") at SP[" << Offset << "]\n");
				MFI->setObjectOffset(*OI, Offset);
				Offset += MFI->getObjectSize(*OI);
			}			
		 }		
		return true;
	}
	