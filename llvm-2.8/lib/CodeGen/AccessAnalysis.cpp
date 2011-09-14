
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
#include "llvm/CodeGen/AccessAnalysis.h"
#include "llvm/CodeGen/MachineMemOperand.h"
#include "llvm/CodeGen/PseudoSourceValue.h"

extern std::map<std::string, std::set<std::string> > g_hFuncCall;
extern const std::map<const Function *, std::map<const BasicBlock *, double> > *g_hF2B2Acc;
extern std::map<const Function *, std::map<const BasicBlock *, std::map<const BasicBlock *, double> > > g_EdgeProbs;

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

static bool InstructionLoadFromFI(const MachineInstr *MI, int FI) {
  for (MachineInstr::mmo_iterator o = MI->memoperands_begin(),
         oe = MI->memoperands_end(); o != oe; ++o) {
    if (!(*o)->isLoad() || !(*o)->getValue())
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
std::map<const Function *, std::map<int, std::map<int, AccessEdge *>  > > hAccRecord; // for computing
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
		hAccRecord[mf.getFunction()].clear();
		AccessCount[mf.getFunction()].clear();
		std::map<int, std::map<int, AccessEdge *> > &AccEdge = hAccRecord[fn];
		//std::map<int, AccessRecord *, RecordCmp> &fAccessCount = AccessCount[mf.getFunction()];
		std::map<int, AccessRecord *> AccessSet;

        std::map<const MachineBasicBlock *, pair<int, bool> > hBasicFirst;
        std::map<const MachineBasicBlock *, pair<int, bool> > hBasicbLast;
		for (MachineFunction::const_iterator BBI = mf.begin(), FE = mf.end();
			BBI != FE; ++BBI)
		{
			// Get access frequence for block

			double dFactor = GetFrequency(BBI, fn);

			int preObj = -1;
			bool bFirst = true;
			bool preRead = true;
			// Get access frequncy for each operand
			MachineBasicBlock::const_iterator MI = BBI->begin(), BBE = BBI->end(), LastI = BBI->end();
			if( LastI != MI)
                -- LastI;
			for (; MI != BBE; ++MI)
			{
				DEBUG(MI->print(dbgs(), NULL ));
                int FI = 0;
                bool bRead = true;
                for (unsigned i = 0, e = MI->getNumOperands(); i != e; ++ i)
                {
                    const MachineOperand &MO = MI->getOperand(i);

                    if ( MO.isFI() )
                    {
                        FI = MO.getIndex();
                        DEBUG(dbgs() << FI << "\n");
                        bRead = false;
                        if( i != 0 )
                            bRead = true;
                        DEBUG(dbgs() << "#####" << bRead << "####:\t" << FI << "\n" );
                        if( bFirst )
                        {
                            hBasicFirst[BBI] = (pair<int, bool>(FI, bRead));
                            bFirst = false;
                            preRead = bRead;
                            preObj = FI;
                            continue;
                        }
                        UpdateEdge(AccEdge, preObj, FI, preRead, bRead, dFactor);
                        preObj = FI;
                        preRead = bRead;
                    }
                }
                if( MI == LastI )
                    hBasicbLast[BBI] = (pair<int, bool>(FI, bRead) );
            }
        }

        // Get the weight across basic blocks

        std::map<const BasicBlock *, std::map<const BasicBlock *, double> > &Edges = g_EdgeProbs[fn];
        for (MachineFunction::const_iterator BBI = mf.begin(), FE = mf.end();
			BBI != FE; ++BBI)
        {
            const BasicBlock *bb = BBI->getBasicBlock();
            if( bb == NULL || hBasicbLast.find(BBI) == hBasicbLast.end())
                continue;
            std::set<const MachineBasicBlock *> succ;
            succ.insert(BBI->succ_begin(), BBI->succ_end());

            std::set<const MachineBasicBlock *>::iterator s_i = succ.begin(), s_e = succ.end();
            for(; s_i != s_e; ++ s_i)
            {
                const MachineBasicBlock *mbb1 = *s_i;
                const BasicBlock *bb1 = mbb1->getBasicBlock();
                double probability = 1.0;
                if( bb1 == NULL || hBasicFirst.find(mbb1) == hBasicFirst.end() )
                    continue;
                if( bb1 == bb)
                {
                    probability = 1.0/succ.size();
                }
                else
                {
                    if( Edges[bb].find(bb1) != Edges[bb].end() )
                        probability = Edges[bb][bb1];
                }
                double dFactor = GetFrequency(BBI, fn);
                UpdateEdge(AccEdge, hBasicbLast[BBI].first, hBasicFirst[mbb1].first,
                           hBasicbLast[BBI].second, hBasicFirst[mbb1].second, dFactor);
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
			Objects.insert(i);
		 }

		 // Get the concerned edges and objects
		const Function *fn = mf.getFunction();
		std::map<int, std::map<int, AccessEdge *> > &fAccRecord = hAccRecord[fn];
		std::map<int, std::map<int, double> > &fGraph = hWeightGraph[fn];
		std::map<int, std::map<int, AccessEdge *> >::iterator acc_p = fAccRecord.begin(), acc_e = fAccRecord.end();
		std::set<int> allocated;
		std::set<int> sorted;
		for(; acc_p != acc_e; ++ acc_p )
		{
			if( Objects.find(acc_p->first) == Objects.end() )      // only consider concerned objects
			{
			    allocated.insert(acc_p->first);
			    continue;
			}
			if(  MFI->getObjectSize(acc_p->first) > CASCH_LINE_SIZE)
			{
			    allocated.insert(acc_p->first);
			    continue;
			}

			/*std::map<int, AccessEdge *>::iterator i2d_p = acc_p->second.begin(), i2d_e = acc_p->second.end();
			for( ; i2d_p != i2d_e; ++ i2d_p)*/
			std::map<int, std::map<int, AccessEdge *> >::iterator acc1_p = fAccRecord.begin(), acc1_e = fAccRecord.end();
			for(; acc1_p != acc1_e; ++ acc1_p )
			{
                if( Objects.find(acc1_p->first) == Objects.end() )        // only consider concerned edges
                {
                    allocated.insert(acc1_p->first);
                    continue;
                }
                if(  MFI->getObjectSize(acc1_p->first) > CASCH_LINE_SIZE)
                {
                    allocated.insert(acc1_p->first);
                    continue;
                }
                if( acc_p->second.find(acc1_p->first) == acc_p->second.end())
                    fGraph[acc_p->first][acc1_p->first] = fGraph[acc1_p->first][acc_p->first] = 0;
                else
                {
                    AccessEdge *pEdge = fAccRecord[acc_p->first][acc1_p->first];
                    if( pEdge == NULL )
                        continue;
                    // compute the edge weight

                    //pEdge->dWeight = pEdge->dCrossWeight + pEdge->nRR * 1 + pEdge->nWW * 4 + pEdge->nRW  * (-1) + pEdge->nWR * (-2);
                    fGraph[acc_p->first][acc1_p->first] = fGraph[acc1_p->first][acc_p->first] = pEdge->dWeight;
                }
			}
		}
		int nTotal = fGraph.size();
		DumpGraph(fGraph);

		 map<int, CCacheBlock *> hAlloc;
		 list<CCacheBlock *> Block_list;
        std::map<int, std::set<int> > removedEdge;

		 int nBlock = 0;
		 bool bNOver = true;
		 while ( bNOver )
		 {
		     DEBUG(errs() << "$$$Starting allocation$$$\n");
			std::map<int, std::map<int, double> >::iterator I = fGraph.begin(), E = fGraph.end();
			 // find the most weight edge
			 int nPrev = 0, nNext = 0;
			 bool bNFound = true;
			 for(; I != E; ++ I)
			 {
				 if (allocated.find(I->first) != allocated.end() )      // the first object in each block is not allcoated ??
					 continue;
				 std::map<int,double> &adjacent = I->second;
				 std::map<int, double>::iterator i2d_p = adjacent.begin(), i2d_e = adjacent.end();
				 for( ; i2d_p != i2d_e; ++ i2d_p)
				 {
					 if (allocated.find(i2d_p->first) != allocated.end() || i2d_p->first == I->first
                        || removedEdge[i2d_p->first].find(I->first) != removedEdge[i2d_p->first].end() )
						continue;
					 if( bNFound || fGraph[nPrev][nNext] < i2d_p->second)
					 {
						 nPrev = I->first;
						 nNext = i2d_p->first;
						 bNFound = false;
					 }
				 }
			 }

			 if( bNFound )
                break;
            // allocate the objects connected by this edge
            // a. alloate both objects
            // b. erase the second object
            // c. set v_current
            // d. merge the edges of second object

             int nOffset = 0;
             if( nBlock == 0 )         // The first block follows the original offset
                nOffset = Offset;
             int nOffset1 = CacheOffset(mf, hAlloc, nOffset, nPrev);
             if( nOffset1 == - 1)
             {
                 if( nBlock == 0 && Offset > 0) // if the first block could not hold the first allocable object, skip it
                 {
                     CCacheBlock *pBlock = new CCacheBlock(nBlock ++, 0);
                     Block_list.push_back(pBlock);
                     pBlock->m_nOffset = Offset;
                     continue;
                 }
                 assert(false && "nOffset1 should not be -1");
             }
             nOffset = nOffset1 + MFI->getObjectSize(nPrev);
             int nOffset2 = CacheOffset(mf, hAlloc, nOffset, nNext);
             if( nOffset2 == -1)  // if the block could not hold the second object of an edge, skip it or split this edge (this edge is not so valuable)
             {
                 if( nBlock == 0 && Offset > 0)
                 {
                     CCacheBlock *pBlock = new CCacheBlock(nBlock ++, 0);
                     Block_list.push_back(pBlock);
                     pBlock->m_nOffset = Offset;
                     continue;
                 }
                 else
                 {
                     removedEdge[nPrev].insert(nNext);
                     removedEdge[nNext].insert(nPrev);
                     continue;
                 }
             }

             // create a new block to handle the most-beneficial edge
             CCacheBlock *pBlock = new CCacheBlock(nBlock ++, 0);
             Block_list.push_back(pBlock);
             pBlock->m_hOff2Obj[nOffset1] = nPrev;
             pBlock->m_nOffset = nOffset1 + MFI->getObjectSize(nPrev);
             pBlock->m_Objs.insert(nPrev);
             sorted.insert(nPrev);
             pBlock->m_hOff2Obj[nOffset2] = nNext;
             pBlock->m_nOffset = nOffset2 + MFI->getObjectSize(nNext);
             pBlock->m_Objs.insert(nNext);
            sorted.insert(nNext);

             allocated.insert(nNext);

             int v_current = nPrev;

             UpdateGraph(fGraph, removedEdge, pBlock, nNext);

            // allocate the adjacent into the same block
            while (true)
            {
                // choose most-beneficial adjacent to v_current
                bool bNFound = true;
                int nBig;
                map<int, double> &adjacents = fGraph[v_current];
                map<int, double>::iterator d_i = adjacents.begin(), d_e = adjacents.end();
                for( ; d_i != d_e; ++ d_i )
                {
                    if (allocated.find(d_i->first) != allocated.end() || d_i->first == v_current)
						continue;
                    if( bNFound || adjacents[nBig] < adjacents[d_i->first])
                    {
                        nBig = d_i->first;
                        bNFound = false;
                    }
                }
                 // allocating the nBig adjacent
                if( bNFound)        // all objects has been allocated
                {
                    bNOver = false;
                    break;
                }

                int nOffset = CacheOffset(mf, hAlloc, pBlock->m_nOffset, nBig);
                if( nOffset == -1)     // this block is full
                {
                    allocated.insert(v_current);
                    break;
                }

                pBlock->m_hOff2Obj[nOffset] = nBig;
                pBlock->m_Objs.insert(nBig);
                pBlock->m_nOffset = nOffset + MFI->getObjectSize(nBig);
                allocated.insert(nBig);
                sorted.insert(nBig);
                UpdateGraph(fGraph, removedEdge, pBlock, nBig);
            }

		 }
		 Offset = AssignOffset(Block_list, Offset, Offset, mf, StackGrowsDown);

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

	int CacheOffset(MachineFunction &mf, std::map<int, CCacheBlock *> &hAlloc, int nOffset, int nPrev)
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
			if( nOffset + MFI->getObjectSize(nPrev) > CASCH_LINE_SIZE )
				return -1;
			DEBUG(dbgs() << "alloc FI(" << nPrev << ") at Block[" << ":" << nOffset << "]\n");
			//pBlock->m_hObj2Off[nPrev] = pBlock->m_nOffset;
			//pBlock->m_nOffset += MFI->getObjectSize(nPrev);
			return nOffset;
		}

	 }

	 int UpdateGraph(std::map<int, std::map<int, double> > &graph, std::map<int, std::set<int> > &removedEdge, CCacheBlock *pBlock, int nIndex)
	 {
		 std::map<int, int>::iterator o2d_I = pBlock->m_hOff2Obj.begin();

		 // the representive element
		 int indexRep = o2d_I->second;

		 // add the edge weight of nIndex onto each edge of the repre-element
		 std::map<int, double>::iterator i2d_I = graph[nIndex].begin(), i2d_E = graph[nIndex].end();
		 for(; i2d_I != i2d_E; ++i2d_I)
		 {
		    if( removedEdge[nIndex].find(i2d_I->first) != removedEdge[nIndex].end() )     // skip the erased edges
                continue;
			graph[indexRep][i2d_I->first] += graph[nIndex][i2d_I->first];
		 }
		  // erase the internal edges when merging
		 //graph[indexRep].erase(nIndex);
		 return 0;
	 }

	 int AssignOffset(std::list<CCacheBlock *> &Block_list, int Offset, int nOriOff, MachineFunction &mf, bool StackGrowsDown)
	 {
		 MachineFrameInfo *MFI = mf.getFrameInfo();

         Offset = 0;
		 std::list<CCacheBlock *>::iterator b_I = Block_list.begin(), b_E = Block_list.end();
		 for(; b_I != b_E; ++ b_I)
		 {

			 Offset = (Offset + CASCH_LINE_SIZE-1) /CASCH_LINE_SIZE * CASCH_LINE_SIZE;
			 CCacheBlock *pBlock = *b_I;
			 int nFinal = 0;
			 std::map<int,int>::iterator i2i_I = pBlock->m_hOff2Obj.begin(), i2i_E = pBlock->m_hOff2Obj.end();
			 for(; i2i_I != i2i_E; ++ i2i_I)
			 {
				if (StackGrowsDown)
				{
					int nOff = i2i_I->first;
					int index = i2i_I->second;
					nFinal = nOff+ MFI->getObjectSize(index)+Offset;
					MFI->setObjectOffset(index, -nFinal); // Set the computed offset
					DEBUG(dbgs() << "qali alloc FI(" << i2i_I->second << ") at SP[" << -nFinal << "]\n");
				}
				else
				{
					int nOff = i2i_I->first;
					int index = i2i_I->second;
					nFinal = nOff+Offset;
					MFI->setObjectOffset(index, nFinal);
					DEBUG(dbgs() << "qali alloc FI(" << i2i_I->second << ") at SP[" << nFinal << "]\n");
				}
			 }
			 Offset = nFinal;
			 if( (*b_I)->m_hOff2Obj.empty() )
		     {
		         Offset = CASCH_LINE_SIZE;
		     }
		 }
		 return Offset;
	 }
	/*bool PackStack(MachineFunction &mf, int64_t &Offset, RegScavenger *RS, int min, int max, SmallSet<int, 16> &LargeStackObjs)
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
	*/
    int UpdateEdge(std::map<int, std::map<int, AccessEdge *> > &AccEdge, int nFirst, int nSec, bool bFirst, bool bSec, double dFreq)
    {
        AccessEdge * pAccEdge = NULL;
        if(AccEdge[nFirst].find(nSec) == AccEdge[nFirst].end())
        {
            pAccEdge = new AccessEdge(nFirst, nSec);
            AccEdge[nFirst][nSec] = pAccEdge;
            AccEdge[nSec][nFirst] = pAccEdge;
        }
        else
        {
            pAccEdge = AccEdge[nFirst][nSec];
        }

        if(bFirst  && bSec)
        {
            //++pAccEdge->nRR;
            pAccEdge->dWeight += dFreq * 10;
        }
        else if( bFirst && !bSec )
            pAccEdge->dWeight += dFreq * (2);
        else if( !bFirst && bSec )
            pAccEdge->dWeight += dFreq * (2);
        else
            pAccEdge->dWeight += dFreq * 200;
        return 0;
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

	int DumpGraph(std::map<int, std::map<int, double> > &graph)
	{
		std::map<int,std::map<int, double> >::iterator i2i_p = graph.begin(), i2i_e = graph.end();
		for(; i2i_p != i2i_e; ++ i2i_p)
		{
			DEBUG(dbgs() << "#" << i2i_p->first << "\t");
			std::map<int, double>::iterator i2d_p= i2i_p->second.begin(), i2d_e = i2i_p->second.end();
			for(; i2d_p != i2d_e; ++ i2d_p)
			{
				DEBUG(dbgs() << "(" << i2d_p->first << "," << i2d_p->second << ")");
			}
			DEBUG(dbgs() << "\n" );
		}
		return 0;
	}
