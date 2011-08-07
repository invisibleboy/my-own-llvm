
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
			
			int preObj = -1;
			bool bFirst = true;
			bool preRead = true;
			// Get access frequncy for each operand
			for (MachineBasicBlock::const_iterator MI = BBI->begin(), BBE = BBI->end();
            MI != BBE; ++MI)
			{
				DEBUG(MI->print(dbgs(), NULL ));	
				/*for (unsigned i = 0, e = MI->getNumOperands(); i != e; ++ i)
				{
					const MachineOperand &MO = MI->getOperand(i);
					int FI = 0;
					if( MO.isFI() )
						FI = MO.getIndex();
					DEBUG(dbgs() << FI << "\n");
				}
				for (MachineInstr::mmo_iterator o = MI->memoperands_begin(),
					 oe = MI->memoperands_end(); o != oe; ++o) 
				{
					//bool bRead = true;
					//const FixedStackPseudoSourceValue *Value = dyn_cast<const FixedStackPseudoSourceValue>((*o)->getValue());
					const FixedStackPseudoSourceValue *Value = dyn_cast< const FixedStackPseudoSourceValue >((*o)->getValue());
					if ( Value )
					{				
						if( !(*o)->isLoad() && !(*o)->isStore() )
							continue;
						int FI = Value->getFrameIndex();
						DEBUG(dbgs() << "#####" << (*o)->isLoad() << "####:\t" << FI << "\n" );;	
						if( preObj == -1 )
						{
							preObj = FI;
							preRead = true;
							continue;
						}
						
						AccessEdge * pAccEdge = NULL;
						if(AccEdge[preObj].find(FI) == AccEdge[preObj].end())
						{
							pAccEdge = new AccessEdge(preObj, FI);
							AccEdge[preObj][FI] = pAccEdge;
							AccEdge[FI][preObj] = pAccEdge;
							//hAccRecord[preObj][FI] = pAccEdge;
							//hAccRecord[FI][preObj] = pAccEdge;
						}
						else
						{
							pAccEdge = AccEdge[preObj][FI];	
						}
						
						if(preRead  && (*o)->isLoad())
							++pAccEdge->nRR;
						else if( preRead && (*o)->isStore() )
							++ pAccEdge->nRW;
						else if( !preRead && (*o)->isLoad() )
							++ pAccEdge->nWR;
						else
							++ pAccEdge->nWW;
					}
				}
			}
			
		}*/
			for (unsigned i = 0, e = MI->getNumOperands(); i != e; ++ i)
			{
				const MachineOperand &MO = MI->getOperand(i);
				
				if ( MO.isFI() )
				{
					int FI = MO.getIndex();
					DEBUG(dbgs() << FI << "\n");
					//if( InstructionStoresToFI(MI,FI) || InstructionLoadFromFI(MI,FI) )
					{
						bool bRead = false;
						if( i != 0 )
							bRead = true;							
						DEBUG(dbgs() << "#####" << bRead << "####:\t" << FI << "\n" );
						if( bFirst )
						{
							bFirst = false;
							preRead = bRead;
							preObj = FI;
							/*if( AccEdge.find(preObj) == AccEdge.end() )
							{
								std::map<int, map<int, AccessEdge *> >::iterator I = AccEdge.begin(), E = AccEdge.end();
								for(; I != E; ++ I)
									
							}*/
							continue;
						}
						
						AccessEdge * pAccEdge = NULL;
						if(AccEdge[preObj].find(FI) == AccEdge[preObj].end())
						{
							pAccEdge = new AccessEdge(preObj, FI);
							AccEdge[preObj][FI] = pAccEdge;
							AccEdge[FI][preObj] = pAccEdge;
						}
						else
						{
							pAccEdge = AccEdge[preObj][FI];	
						}
						preObj = FI;
						preRead = bRead;
						if(preRead  && bRead)
							++pAccEdge->nRR;
						else if( preRead && !bRead )
							++ pAccEdge->nRW;
						else if( !preRead && bRead )
							++ pAccEdge->nWR;
						else
							++ pAccEdge->nWW;
					}
				}
			}
		}		
	}
		
		// copy into access record list in ascend sort
/*		std::map<int, std::set<AccessEdge *, EdgeCmp> > &fAccList = hAccList[fn];
		std::map<int, std::map<int, AccessEdge *> >::iterator acc_p = AccEdge.begin(), acc_e = AccEdge.end();
		for(; acc_p != acc_e; ++ acc_p )
		{
			std::map<int, AccessEdge *>::iterator i2a_p = acc_p->second.begin(), i2a_e = acc_p->second.end();
			for( ; i2a_p != i2a_e; ++ i2a_p)
			{
				if( i2a_p->second == NULL )
					continue;
				// compute the edge weight
				AccessEdge *pEdge = i2a_p->second;
				pEdge->dWeight = pEdge->nRR * 1 + pEdge->nWW * 2 + pEdge->nRW  * (-1) + pEdge->nWR * (-2);
				fAccList[acc_p->first].insert(pEdge);				
			}
		}*/
		
		// copy into access counter in descend sort
		/*std::set<AccessRecord *, RecordCmp> &fAccessCount = AccessCount[mf.getFunction()];
		std::map<int, AccessRecord *>::iterator AI = AccessSet.begin(), AE = AccessSet.end();
		for( ; AI != AE; ++ AI)
		{
			fAccessCount.insert(AI->second );
		}*/
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
		 
		 // sort the edges by weight counter
		const Function *fn = mf.getFunction();		
		std::map<int, std::map<int, AccessEdge *> > &fAccRecord = hAccRecord[fn];
		std::map<int, std::map<int, double> > &fGraph = hWeightGraph[fn];
		std::map<int, std::map<int, AccessEdge *> >::iterator acc_p = fAccRecord.begin(), acc_e = fAccRecord.end();
		int nTotal = fGraph.size();
		std::set<int> allocated;
		std::set<int> sorted;
		for(; acc_p != acc_e; ++ acc_p )
		{
			if( Objects.find(acc_p->first) == Objects.end() )
				allocated.insert(acc_p->first);
			if(  MFI->getObjectSize(acc_p->first) )
				allocated.insert(acc_p->first);
				
			
			/*std::map<int, AccessEdge *>::iterator i2d_p = acc_p->second.begin(), i2d_e = acc_p->second.end();
			for( ; i2d_p != i2d_e; ++ i2d_p)*/
			std::map<int, std::map<int, AccessEdge *> >::iterator acc1_p = fAccRecord.begin(), acc1_e = fAccRecord.end();
			for(; acc1_p != acc1_e; ++ acc1_p ) {
			if( acc_p->second.find(acc1_p->first) == acc_p->second.end())
				fGraph[acc_p->first][acc1_p->first] = 0;
			else
			{
				AccessEdge *pEdge = fAccRecord[acc_p->first][i2d_p->first];
				if( pEdge == NULL )
					continue;
				// compute the edge weight
				
				pEdge->dWeight = pEdge->nRR * 1 + pEdge->nWW * 4 + pEdge->nRW  * (-1) + pEdge->nWR * (-2);
				fGraph[acc_p->first][i2d_p->first] = pEdge->dWeight;
			}
			}
		}
		DumpGraph(fGraph);
		 map<int, CCacheBlock *> hAlloc;
		 list<CCacheBlock *> Block_list;
		 
		 int nBlock = 0;
		 while ( nTotal > 0 )
		 {
			std::map<int, std::map<int, double> >::iterator I = fGraph.begin(), E = fGraph.end();			
			 // find the most weight edge
			 int nPrev = 0, nNext = 0;
			 bool bFirst = true;
			 for(; I != E; ++ I)
			 {
				 if (allocated.find(I->first) != allocated.end())
					 continue;
				 std::map<int,double> &adjacent = I->second;
				 std::map<int, double>::iterator i2d_p = adjacent.begin(), i2d_e = adjacent.end();
				 for( ; i2d_p != i2d_e; ++ i2d_p)
				 {
					 if (allocated.find(i2d_p->first) != allocated.end())
						continue;
					 if( bFirst || fGraph[nPrev][nNext] < i2d_p->second)
					 {
						 nPrev = I->first;
						 nNext = i2d_p->first;
						 bFirst = false;
					 }					
				 }					 
			 }
			 
			 // allocate the objects connected by this edge 
			 if( !bFirst )
			 {
				 bool bSucc = false;
				 CCacheBlock *pBlock = NULL;				
				 if( allocated.find( nPrev) != allocated.end() )
				 {					 
					 pBlock = hAlloc[nPrev];					
					int nOffset = CacheAlloc(mf, hAlloc, pBlock->m_nOffset, nNext);
					if(nOffset != -1 )
					{
						-- nTotal;
						hAlloc[nNext] = pBlock;
						sorted.insert(nNext);
						bSucc = true;
						allocated.insert(nNext);						
						pBlock->m_hOff2Obj[nOffset] = nNext;
						pBlock->m_nOffset = nOffset + MFI->getObjectSize(nNext);
						UpdateGraph(fGraph, pBlock, nNext);
					}
				 }
				 else if( allocated.find(nNext) != allocated.end() )
				 {					 
					 assert(pBlock == NULL);
					 pBlock = hAlloc[nNext];			
					 
					 int nOffset = CacheAlloc(mf, hAlloc, pBlock->m_nOffset, nPrev);
					if(nOffset != -1 )
					{
						-- nTotal;
						hAlloc[nPrev] = pBlock;
						sorted.insert(nPrev);
						bSucc = true;
						allocated.insert(nPrev);						
						pBlock->m_hOff2Obj[nOffset] = nPrev;
						pBlock->m_nOffset = nOffset + MFI->getObjectSize(nPrev);
						UpdateGraph(fGraph, pBlock, nPrev);
					}
				 }
				 else
				 {
					 -- nTotal; -- nTotal;					 
					 pBlock = new CCacheBlock(nBlock ++, 0);
					 Block_list.push_back(pBlock);
					int nOffset1 = CacheAlloc(mf, hAlloc, pBlock->m_nOffset, nPrev), nOffset2;
					if( nOffset1 != -1 )
						nOffset2 = CacheAlloc(mf, hAlloc, nOffset1+MFI->getObjectSize(nPrev), nNext);
					
					 if( nOffset1 != -1 && nOffset2 != -1 )
					 {
						 bSucc = true;
						 sorted.insert(nNext);
						 sorted.insert(nPrev);
						 hAlloc[nPrev] = hAlloc[nNext] = pBlock;
						 allocated.insert(nPrev);
						 allocated.insert(nNext);
						 pBlock->m_hOff2Obj[nOffset1] = nPrev;
						 pBlock->m_hOff2Obj[nOffset2] = nNext;						 
						 pBlock->m_nOffset = nOffset2 + MFI->getObjectSize(nNext);
						 UpdateGraph(fGraph, pBlock, nNext);
					 }			
				 }			
				
				 
			 }
		 }
		 Offset = AssignOffset(Block_list, Offset, mf, StackGrowsDown);
		 
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
	
	int CacheAlloc(MachineFunction &mf, std::map<int, CCacheBlock *> &hAlloc, int nOffset, int nPrev)
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
			if( nOffset + MFI->getObjectSize(nPrev) >= CASCH_LINE_SIZE )
				return -1;
			DEBUG(dbgs() << "alloc FI(" << nPrev << ") at Block[" << ":" << nOffset << "]\n");			
			//pBlock->m_hObj2Off[nPrev] = pBlock->m_nOffset;
			//pBlock->m_nOffset += MFI->getObjectSize(nPrev);
			return nOffset;
		}			
		
	 }
	 
	 int UpdateGraph(std::map<int, std::map<int, double> > &graph, CCacheBlock *pBlock, int nIndex)
	 {		 
		 std::map<int, int>::iterator o2d_I = pBlock->m_hOff2Obj.begin();
			 
		 // the representive element
		 int index1 = o2d_I->second;
		 // erase the internal edges when merging
		 graph[index1].erase(nIndex);
		 graph[nIndex].erase(index1);
		 
		 // add the edge weight of nIndex onto each edge of the rep-element
		 std::map<int, double>::iterator i2d_I = graph[nIndex].begin(), i2d_E = graph[nIndex].end();
		 for(; i2d_I != i2d_E; ++i2d_I)
		 {
			 if( graph[index1].find(i2d_I->first) != graph[index1].end() )
				 graph[index1][i2d_I->first] += graph[nIndex][i2d_I->first];
			else 
				graph[index1][i2d_I->first] = graph[nIndex][i2d_I->first];
		 }
		 return 0;
	 }
	 
	 int AssignOffset(std::list<CCacheBlock *> &Block_list, int nOffset, MachineFunction &mf, bool StackGrowsDown)
	 {		
		 MachineFrameInfo *MFI = mf.getFrameInfo();
		 std::list<CCacheBlock *>::iterator b_I = Block_list.begin(), b_E = Block_list.end();
		 for(; b_I != b_E; ++ b_I)
		 {
			  nOffset = (nOffset + CASCH_LINE_SIZE) /CASCH_LINE_SIZE * CASCH_LINE_SIZE;
			 CCacheBlock *pBlock = *b_I;
			int nFinal = nOffset;
			 std::map<int,int>::iterator i2i_I = pBlock->m_hOff2Obj.begin(), i2i_E = pBlock->m_hOff2Obj.end();
			 for(; i2i_I != i2i_E; ++ i2i_I)
			 {
				if (StackGrowsDown) 
				{				
					int nOff = i2i_I->first;
					int index = i2i_I->second;
					nFinal = nOff+ MFI->getObjectSize(index)+nOffset;
					MFI->setObjectOffset(index, -nFinal); // Set the computed offset
					DEBUG(dbgs() << "alloc FI(" << i2i_I->second << ") at SP[" << -nFinal << "]\n");
				} 
				else
				{					
					int nOff = i2i_I->first;
					int index = i2i_I->second;
					nFinal = nOff+nOffset;
					MFI->setObjectOffset(index, nFinal);				
					DEBUG(dbgs() << "alloc FI(" << i2i_I->second << ") at SP[" << nFinal << "]\n");
				}			
			 }
			 nOffset = nFinal;
		 }
		 return nOffset;
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
	