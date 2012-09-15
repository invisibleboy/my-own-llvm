
// add by qali: MCDL for hybrid cache 
#define DEBUG_TYPE "ACCESS"

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
#include "llvm/CodeGen/AccessAnalysis2.h"
#include "llvm/CodeGen/MachineMemOperand.h"
#include "llvm/CodeGen/PseudoSourceValue.h"
#include "llvm/Support/CommandLine.h"

#ifdef HYBRID_ALLOCATION

extern std::map<std::string, std::set<std::string> > g_hFuncCall;
extern const std::map<const Function *, std::map<const BasicBlock *, double> > *g_hF2B2Acc;
extern std::map<const Function *, std::map<const BasicBlock *, std::map<const BasicBlock *, double> > > g_EdgeProbs;
std::map<const Function *, std::map<int, double> > g_hF2Locks;

std::map<const Function *, std::set<AccessRecord *, RecordCmp> > AccessCount; 		// For access frequency
std::map<Function *, std::map<int, int64_t> > StackLayout;		// For stack layout
std::map<const Function *, std::map<int, double> > hAccFreq; // for computing
std::map<const Function *, std::map<int, std::set<AccessEdge *, EdgeCmp> > > hAccList;   // for storage
std::map<const Function *, std::map<int, std::map<int, double> > > hWeightGraph;


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
		std::map<int, double > &AccFreq = hAccFreq[fn];
		
		for (MachineFunction::const_iterator BBI = mf.begin(), FE = mf.end();
			BBI != FE; ++BBI)
		{
			// Get access frequence for block

			double dFactor = GetFrequency(BBI, fn);

			
			// Get access frequncy for each operand
			MachineBasicBlock::const_iterator MI = BBI->begin(), BBE = BBI->end(), LastI = BBI->end();
			if( LastI != MI)
                -- LastI;
			for (; MI != BBE; ++MI)
			{
				DEBUG(MI->print(dbgs(), NULL ));
                int FI = 0;
                bool bRead = true;
                int e = MI->getNumOperands();
                for (int i = e-1; i >= 0; -- i)
                {
                    const MachineOperand &MO = MI->getOperand(i);

                    if ( MO.isFI() )
                    {
                        FI = MO.getIndex();
                        DEBUG(dbgs() << FI << "\n");
                        bRead = false;
                        if( i != 0 )
                            bRead = true;
                        DEBUG(dbgs() << "#####" << bRead << "####" << dFactor << ":\t" << FI << "\n" );
                        
                        if( !bRead )
						{
							AccFreq[FI] += dFactor;
						}						
						
                    }
                }
                    
            }
        }
        
		return true;
	}

	bool PackStack(MachineFunction &mf, int64_t &Offset, RegScavenger *RS, int min, int max, SmallSet<int, 16> &LargeStackObjs)
	{
		const TargetFrameInfo &TFI = *mf.getTarget().getFrameInfo();
		bool StackGrowsDown = TFI.getStackGrowthDirection() == TargetFrameInfo::StackGrowsDown;
		MachineFrameInfo *MFI = mf.getFrameInfo();


		set<int> Objects;
		DEBUG(errs() << "<<<<<<<Begin " << mf.getFunction()->getName() << ">>>>>>" );
		DEBUG(errs() << min <<"---" << max << "\n");
	
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
			if( MFI->getObjectSize(i) >= CACHE_LINE_SIZE )
				continue;
			Objects.insert(i);
		 }
		 
		 
		const Function *fn = mf.getFunction();
		std::map<int, double> AccFreq = hAccFreq[fn];
		std::set<AccessRecord, RecordCmp> accRecords;
		std::set<int> allocated;
		std::set<int> sorted;
		int nStackSize = 0;
		std::map<int, double>::iterator i2d_p = AccFreq.begin(), i2d_e = AccFreq.end();
		for(; i2d_p != i2d_e; ++ i2d_p)
		{
			int FI = i2d_p->first;
			if( Objects.find(FI) == Objects.end() )
			{
				allocated.insert(FI);
				continue;
			}
			 nStackSize += MFI->getObjectSize(FI);
			 
			 AccessRecord ac;
			 ac.m_nID = FI;
			 ac.m_dCount = i2d_p->second;
			 accRecords.insert(ac);
		}

		 // allocation
		int nBlocks = 0;
		std::list<CCacheBlock *> Block_list;
        std::map<int, CCacheBlock *> hBlocks;     // mapping from data object to the block holding it
		nBlocks = nStackSize/CACHE_LINE_SIZE;
        if( nBlocks < 1)
            nBlocks = 1;		
		int nBlock = 0;
        // 1. initialize the block list, initialize the first block's offset
        for(int i = 0; i < nBlocks; ++ i)
        {
            CCacheBlock *cb = new CCacheBlock(nBlock ++, 0);
            Block_list.push_back(cb);
        }
		DEBUG(errs() << "$$$Starting allocation$$$\n");		
				
		CCacheBlock *headBlock = Block_list.front();
        headBlock->m_nOffset = Offset;
		std::set<AccessRecord, RecordCmp>::iterator a_p = accRecords.begin(), a_e = accRecords.end();
		for(; a_p != a_e; ++ a_p)
		{
			int FI = a_p->m_nID;
			double dFactor = a_p->m_dCount;
			CCacheBlock *pCurBlock = NULL;
			std::list<CCacheBlock *>::iterator c_p = Block_list.begin(), c_e = Block_list.end();
			for(; c_p != c_e; ++ c_p)
			{
				pCurBlock = *c_p;
				int nOffset = CacheOffset(mf, pCurBlock, pCurBlock->m_nOffset, FI);
				if( nOffset == -1)
					continue;
				else
				{
					DEBUG(dbgs() << "alloc FI(" << FI << ") at Block[" << ":" << pCurBlock->m_nID << "][" << nOffset << "] with w-freq [" << dFactor<< "]\n");
					FiniAllocate(FI, nOffset, pCurBlock, hBlocks,  mf, allocated, sorted);
					break;
				}
			}
			
		}        

        // 3. Adjust the stack layout for these memory blocks
        Offset = AssignOffset(Block_list, Offset, Offset, mf, StackGrowsDown);

		 // 4. Alloate the stack objects unsorted
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
				DEBUG(dbgs() << "alloc unsorted FI(" << *OI << ") at SP[" << -Offset << "]\n");
				MFI->setObjectOffset(*OI, -Offset); // Set the computed offset
			}
			else
			{
				DEBUG(dbgs() << "alloc unsorted FI(" << *OI << ") at SP[" << Offset << "]\n");
				MFI->setObjectOffset(*OI, Offset);
				Offset += MFI->getObjectSize(*OI);
			}
		 }
		return true;
	}

    int FiniAllocate(int index, int nOffset, CCacheBlock *pBlock, std::map<int, CCacheBlock *> &hBlocks,
                     llvm::MachineFunction &mf, std::set<int> &allocated, std::set<int> &sorted )
    {
        MachineFrameInfo *MFI = mf.getFrameInfo();
        pBlock->m_hOff2Obj[nOffset] = index;
        pBlock->m_Objs.insert(index);
        pBlock->m_nOffset = nOffset + MFI->getObjectSize(index);
        allocated.insert(index);
        sorted.insert(index);
        hBlocks[index] = pBlock;
//        UpdateGraph(fGraph, removedEdge, pBlock, index);
//          hBlocks;
        return 0;
    }
	int CacheOffset(llvm::MachineFunction &mf, CCacheBlock *pBlock, int nOffset, int nPrev)
	{
		//const TargetFrameInfo &TFI = *mf.getTarget().getFrameInfo();
		//bool StackGrowsDown = TFI.getStackGrowthDirection() == TargetFrameInfo::StackGrowsDown;
		MachineFrameInfo *MFI = mf.getFrameInfo();
		//Function *fn = mf.getFunction();

		 //if (StackGrowsDown)
			//nOffset += MFI->getObjectSize(nPrev);
		 unsigned Align = MFI->getObjectAlignment(nPrev );
		 nOffset = (nOffset + Align - 1)/ Align * Align;
		{
			if( nOffset + MFI->getObjectSize(nPrev) > CACHE_LINE_SIZE )
				return -1;			
			//pBlock->m_hObj2Off[nPrev] = pBlock->m_nOffset;
			//pBlock->m_nOffset += MFI->getObjectSize(nPrev);
			return nOffset;
		}

	 }

    
	 int AssignOffset(std::list<CCacheBlock *> &Block_list, int Offset, int nOriOff, MachineFunction &mf, bool StackGrowsDown)
	 {
		 MachineFrameInfo *MFI = mf.getFrameInfo();

         Offset = 0;
		 std::list<CCacheBlock *>::iterator b_I = Block_list.begin(), b_E = Block_list.end();
		 for(; b_I != b_E; ++ b_I)
		 {
		     CCacheBlock *pBlock = *b_I;
            if( pBlock->m_hOff2Obj.empty() )
		     {
		         // For the first block, align with CACHE_LINE_SIZE; for other empty blocks, skip it
		         if( pBlock->m_nOffset != 0 )   // It is only possible for the first block
				 {
                    Offset = pBlock->m_nOffset;
				 }
				 continue;
		     }

			 int nFinal = (Offset + CACHE_LINE_SIZE-1) /CACHE_LINE_SIZE * CACHE_LINE_SIZE;
			 std::map<int,int>::iterator i2i_I = pBlock->m_hOff2Obj.begin(), i2i_E = pBlock->m_hOff2Obj.end();
			 for(; i2i_I != i2i_E; ++ i2i_I)
			 {
				if (StackGrowsDown)
				{
					int nOff = i2i_I->first;
					int index = i2i_I->second;
					nFinal += nOff+ MFI->getObjectSize(index);
					MFI->setObjectOffset(index, -nFinal); // Set the computed offset
					DEBUG(dbgs() << "qali alloc FI(" << i2i_I->second << ") at SP[" << -nFinal << "]\n");
				}
				else
				{
					int nOff = i2i_I->first;
					int index = i2i_I->second;
					nFinal += nOff;
					MFI->setObjectOffset(index, nFinal);
					DEBUG(dbgs() << "qali alloc FI(" << i2i_I->second << ") at SP[" << nFinal << "]\n");
				}
			 }
			 Offset = nFinal;

		 }
		 return Offset;
	 }
    
    double GetFrequency(const MachineBasicBlock *mbb, const Function *fn)
    {
        double dFactor = 1.0;
        const BasicBlock *bb = mbb->getBasicBlock();
        if( bb != NULL )
        {
            std::map<const Function *, std::map<const BasicBlock *, double> >::const_iterator f2b2acc_p, E = g_hF2B2Acc->end();
            if( (f2b2acc_p = g_hF2B2Acc->find(fn) ) != E )
            {
                std::map<const BasicBlock *, double>::const_iterator b2acc_p, EE = f2b2acc_p->second.end();
                if( (b2acc_p = f2b2acc_p->second.find(bb) ) != EE )
                    dFactor = b2acc_p->second;
            }
        }
        if( dFactor == 0.0)
            dFactor = 1.0;
        return dFactor;
    }


	bool CacheLock(llvm::MachineFunction &mf)
	{
		const llvm::Function *fn = mf.getFunction();
		MachineFrameInfo *MFI = mf.getFrameInfo();
		
		DEBUG(dbgs() << "##### begin cache locking " << fn->getName() << "####\n" );
		
		map<int, double> &locks = g_hF2Locks[fn];		
		
		//std::map<int, AccessRecord *, RecordCmp> &fAccessCount = AccessCount[mf.getFunction()];

        std::map<int, std::map<const MachineBasicBlock *, bool> > hBasicFirst, hBasicLast, hPreOp;
		for (MachineFunction::const_iterator BBI = mf.begin(), FE = mf.end();
			BBI != FE; ++BBI)
		{
			// Get access frequence for block

			double dFactor = GetFrequency(BBI, fn);
			
			// Get access frequncy for each operand
			MachineBasicBlock::const_iterator MI = BBI->begin(), BBE = BBI->end(), LastI = BBI->end();
			if( LastI != MI)
                -- LastI;
			for (; MI != BBE; ++MI)
			{
				DEBUG(MI->print(dbgs(), NULL ));
                int FI = 0;
                bool bRead = true;
				int e = MI->getNumOperands();
                for (int i = e-1; i >= 0; -- i)
                {
                    const MachineOperand &MO = MI->getOperand(i);

                    if ( MO.isFI() )
                    {
                        FI = MO.getIndex();
						int blockID = -MFI->getObjectOffset(FI)/CACHE_LINE_SIZE;
                        bRead = false;
                        if( i != 0 )
                            bRead = true;
                        if( !bRead)
							locks[blockID] += dFactor;						
                    }
                }
                    
            }
        }
		return true;
	}
#endif