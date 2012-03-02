
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

cl::opt<int> Alpha("alpha", cl::desc("The alpha value") );
cl::opt<int> Beta("beta", cl::desc("The beta value") );

extern std::map<std::string, std::set<std::string> > g_hFuncCall;
extern const std::map<const Function *, std::map<const BasicBlock *, double> > *g_hF2B2Acc;
extern std::map<const Function *, std::map<const BasicBlock *, std::map<const BasicBlock *, double> > > g_EdgeProbs;

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
                        DEBUG(dbgs() << "#####" << bRead << "####" << dFactor << ":\t" << FI << "\n" );
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
		int estStackSize = 0;
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
            estStackSize += MFI->getObjectSize(i);
			Objects.insert(i);
		 }

		 // Get the concerned edges and objects, namely, those contained in the graph
		const Function *fn = mf.getFunction();
		std::map<int, std::map<int, AccessEdge *> > &fAccRecord = hAccRecord[fn];
		std::map<int, std::map<int, double> > &fGraph = hWeightGraph[fn];
		std::map<int, std::map<int, AccessEdge *> >::iterator acc_p = fAccRecord.begin(), acc_e = fAccRecord.end();
		std::set<int> allocated;
		std::set<int> sorted;
		int nStackSize = 0;
		for(; acc_p != acc_e; ++ acc_p )
		{
			if( Objects.find(acc_p->first) == Objects.end() )      // remove unconcerned objects
			{
			    allocated.insert(acc_p->first);
			    continue;
			}
			if(  MFI->getObjectSize(acc_p->first) > CACHE_LINE_SIZE)
			{
			    allocated.insert(acc_p->first);
			    continue;
			}

            nStackSize += MFI->getObjectSize(acc_p->first);

			/*std::map<int, AccessEdge *>::iterator i2d_p = acc_p->second.begin(), i2d_e = acc_p->second.end();
			for( ; i2d_p != i2d_e; ++ i2d_p)*/
			std::map<int, std::map<int, AccessEdge *> >::iterator acc1_p = fAccRecord.begin(), acc1_e = fAccRecord.end();
			for(; acc1_p != acc1_e; ++ acc1_p )
			{
                if( Objects.find(acc1_p->first) == Objects.end() )        // remove unconcerned edges
                {
                    allocated.insert(acc1_p->first);
                    continue;
                }
                if(  MFI->getObjectSize(acc1_p->first) > CACHE_LINE_SIZE)
                {
                    allocated.insert(acc1_p->first);
                    continue;
                }

                if( acc_p->second.find(acc1_p->first) == acc_p->second.end())
                    fGraph[acc_p->first][acc1_p->first] = fGraph[acc1_p->first][acc_p->first] = 0;
                else
                {
                    AccessEdge *pEdge = fAccRecord[acc_p->first][acc1_p->first];
                    assert(pEdge);
                    if( pEdge == NULL )
                        continue;
                    // compute the edge weight

                    //pEdge->dWeight = pEdge->dCrossWeight + pEdge->nRR * 1 + pEdge->nWW * 4 + pEdge->nRW  * (-1) + pEdge->nWR * (-2);
                    fGraph[acc_p->first][acc1_p->first] = fGraph[acc1_p->first][acc_p->first] = pEdge->dWeight;
                }
			}
		}
		DumpGraph(fGraph);

        // Only the intersection of Objects (which is allocable) and fAccRecord (which is collected) are in fGraph
        // Now, begin the allocation process
        int nBlock = 0;
		std::list<CCacheBlock *> Block_list;
        std::map<int, std::set<int> > removedEdge;
        std::map<int, CCacheBlock *> hBlocks;     // mapping from data object to the block holding it
        std::set<int> headVertice;
        int nBlocks = nStackSize/CACHE_LINE_SIZE;
        if( nBlocks < 1)
            nBlocks = 1;
        // 1. initialize the block list, initialize the first block's offset
        for(int i = 0; i < nBlocks; ++ i)
        {
            CCacheBlock *cb = new CCacheBlock(nBlock ++, 0);
            Block_list.push_back(cb);
        }
        CCacheBlock *headBlock = Block_list.front();
        headBlock->m_nOffset = Offset;

        // 2. Do  allocation for the N-block list
        DEBUG(errs() << "$$$Starting allocation$$$\n");
        do
        {
            // 2.1 find the most beneficial edge
            std::map<int, std::map<int, double> >::iterator I = fGraph.begin(), E = fGraph.end();
			 int nPrev = 0, nNext = 0;
			 bool bNFound = true;
			 for(; I != E; ++ I)
			 {
			     // the first object in each block is not allcoated ?? No, alloated
			     // skip allocated && non-head vertex
			     bool bHeadVertex = false;
			     if( headVertice.find(I->first) != headVertice.end() )
                    bHeadVertex = true;
				 if (allocated.find(I->first) != allocated.end() && !bHeadVertex )
					 continue;

				 std::map<int,double> &neighbors = I->second;
				 std::map<int, double>::iterator i2d_p = neighbors.begin(), i2d_e = neighbors.end();
				 for( ; i2d_p != i2d_e; ++ i2d_p)
				 {
				     // skip the second headVertice
				     bool bHeadVertex2 = false;
                     if( headVertice.find(i2d_p->first) != headVertice.end() )
                        bHeadVertex2= true;
				     if( bHeadVertex && bHeadVertex2 )
                        continue;
				     // skip the allcoated vertex, the removed edges, and the self-edge
					 if ( (allocated.find(i2d_p->first) != allocated.end() && !bHeadVertex2)
                        || i2d_p->first == I->first
                        || removedEdge[i2d_p->first].find(I->first) != removedEdge[i2d_p->first].end() )
						continue;
					 if( bNFound || i2d_p->second > fGraph[nPrev][nNext] )  // record the bigger one
					 {
						 nPrev = I->first;
						 nNext = i2d_p->first;
						 bNFound = false;
					 }
				 }
			}
            if( bNFound)   // no more edges left, it means all objects has been allocated or no more could be allocated
            {
                break;
            }

            CCacheBlock* pBlock = NULL;
            if( headVertice.find(nPrev) != headVertice.end() )    // nPrev is the head vertex
            {
                assert(headVertice.find(nNext) == headVertice.end() );      // ????? with line 258
                pBlock = SingleAllocate(nPrev,nNext, hBlocks, mf, allocated, sorted);
                if( pBlock )
                {
                    UpdateGraph(fGraph, removedEdge, pBlock, nNext);
                }

            }
            else if( headVertice.find(nNext) != headVertice.end() )   // nNext is the head vertex
            {
                assert(headVertice.find(nPrev) == headVertice.end() );      // ????? with line 258
                pBlock = SingleAllocate(nNext,nPrev, hBlocks, mf, allocated, sorted);
                if( pBlock )
                {
                    UpdateGraph(fGraph, removedEdge, pBlock, nPrev);
                }
            }
            else
            {
                pBlock = DoubleAllocate(nPrev,nNext, hBlocks, Block_list, mf, allocated, sorted);
                if( pBlock )
                {
                    std::map<int,int>::iterator i2i_p = pBlock->m_hOff2Obj.begin();
                    if( i2i_p->second != nPrev )
                        UpdateGraph(fGraph, removedEdge, pBlock, nPrev);
                    else
                        headVertice.insert( nPrev);               // add a new head vertex
                    UpdateGraph(fGraph, removedEdge, pBlock, nNext);
                }

            }

            if( !pBlock )
            {
                removedEdge[nPrev].insert(nNext);
                removedEdge[nNext].insert(nPrev);
            }
        }while(true);

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



    CCacheBlock* SingleAllocate(int first, int second, std::map<int, CCacheBlock *> &hBlocks, llvm::MachineFunction &mf,
                        std::set<int> &allocated, std::set<int> &sorted)
    {
        CCacheBlock *pBlock = hBlocks[first];

        int nOffset = CacheOffset(mf, pBlock, pBlock->m_nOffset, second);
        if( nOffset == -1)
        {
            return NULL;
        }
        else
        {
            FiniAllocate(second, nOffset, pBlock, hBlocks,  mf, allocated, sorted);
            return pBlock;
        }
    }

    CCacheBlock* DoubleAllocate(int first, int second, std::map<int, CCacheBlock *> &hBlocks, std::list<CCacheBlock *> &Blocks, llvm::MachineFunction &mf,
                        std::set<int> &allocated, std::set<int> &sorted)
    {
        std::list<CCacheBlock *>::iterator cb_p = Blocks.begin(), cb_e = Blocks.end();
        MachineFrameInfo *MFI = mf.getFrameInfo();

        bool bAllocable = false;
        for(; cb_p != cb_e; ++ cb_p)
        {
            CCacheBlock *cb = *cb_p;
            int nOffset1 = CacheOffset(mf, cb, cb->m_nOffset, first);
            if( nOffset1 == -1 )
                continue;
            int nOffset2 = CacheOffset(mf, cb, nOffset1 + MFI->getObjectSize(first), second);
            if( nOffset2 == -1)
                continue;

            bAllocable = true;
            FiniAllocate(first, nOffset1, cb, hBlocks, mf, allocated, sorted);
            FiniAllocate(second, nOffset2, cb, hBlocks, mf, allocated, sorted);
            break;
        }
        if( bAllocable)
            return *cb_p;
        else
            return NULL;
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
			DEBUG(dbgs() << "try alloc FI(" << nPrev << ") at Block[" << ":" << pBlock->m_nID << "][" << nOffset << "]\n");
			//pBlock->m_hObj2Off[nPrev] = pBlock->m_nOffset;
			//pBlock->m_nOffset += MFI->getObjectSize(nPrev);
			return nOffset;
		}

	 }

     bool FindLeastEdge(int &nPrev, int &nNext, std::map<int, std::map<int, double> > &fGraph, std::map<int, std::set<int> > &removedEdge, std::set<int> &allocated)
     {
         std::map<int, std::map<int, double> >::iterator I = fGraph.begin(), E = fGraph.end();
			 // find the most weight edge   ==> least weight edge
         nPrev = 0;
         nNext = 0;
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
                 if( bNFound || fGraph[nPrev][nNext] > i2d_p->second)     // the least edge
                 {
                     nPrev = I->first;
                     nNext = i2d_p->first;
                     bNFound = false;
                 }
             }
         }
         return bNFound;
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
		     CCacheBlock *pBlock = *b_I;
            if( pBlock->m_hOff2Obj.empty() )
		     {
		         // For the first block, align with CACHE_LINE_SIZE; for other empty blocks, skip it
		         if( pBlock->m_nOffset != 0 )   // It is only possible for the first block
				 {
                    Offset = (pBlock->m_nOffset + CACHE_LINE_SIZE-1) /CACHE_LINE_SIZE * CACHE_LINE_SIZE;
					continue;
				 }
		     }

			 Offset = (Offset + CACHE_LINE_SIZE-1) /CACHE_LINE_SIZE * CACHE_LINE_SIZE;
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

		 }
		 return Offset;
	 }

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
            pAccEdge->dWeight += dFreq * Alpha;   //10
        }
        else if( bFirst && !bSec )
            pAccEdge->dWeight += dFreq * Beta;  // 2
        else if( !bFirst && bSec )
            pAccEdge->dWeight += dFreq * Beta;  // 2
        else
            pAccEdge->dWeight += dFreq * Alpha;  // 200
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
