#ifndef __ACCESS_ANALYSIS_H_
#define __ACCESS_ANALYSIS_H_

#include <vector>
#include <map>
#include <set>
#include <list>
#include "llvm/ADT/SmallSet.h"
using namespace std;

#include "llvm/Function.h"
#include "llvm/CodeGen/MachineFunction.h"
using namespace llvm;

#define CASCH_LINE_SIZE	64
#define ASSOCIATIVITY	8
#define NUM_OF_SETS	1024
#define	CASCH_CAPACITY	(2 << 20)
#define NOF (-(2<<31))


namespace llvm
{
	class RegScavenger;
}


//namespace StackLayout
//{
	//typedef long long int64_t;
	// data structure for storage
	struct AccessRecord
	{
		int m_nID;
		double m_dCount;

	};


	class AccessEdge
	{
		public:
		int pPrev;
		int pNext;
		int nRR;
		int nRW;
		int nWR;
		int nWW;
		double dWeight;
		double dCrossWeight;

	public:
		AccessEdge(int prev, int next)
		{
			pPrev = prev;
			pNext = next;
			nRR = nRW = nWR = nWW = 0;
			dWeight = 0.0;
            dCrossWeight = 0.0;
		}
	};

	class CCacheBlock
	{
	public:
		int m_nID;
		//int m_nLeft;
		int m_nOffset;
		std::map<int, int> m_hOff2Obj;
		std::set<int> m_Objs;

		CCacheBlock(int nID, int offset) {m_nID = nID; m_nOffset = offset;}

	};
	struct EdgeCmp
	{

		bool operator () ( AccessEdge *first, AccessEdge *second)
		{
			if( first->dWeight > second->dWeight)
				return true;
			return false;
		}
	};
	struct RecordCmp
	{
		bool operator () ( AccessRecord *first, AccessRecord *second)
		{
			if( first->m_dCount < second->m_dCount)
				return true;
			return false;
		}
	};

	extern std::map<const Function *, std::map<int, std::map<int, AccessEdge *>  > > hAccRecord; // for computing
	extern std::map<const Function *, std::map<int, std::set<AccessEdge *, EdgeCmp> > > hAccList;   // for storage
	extern std::map<const Function *, std::map<int, std::map<int, double> > > hWeightGraph;

	extern std::map<const Function *, std::set<AccessRecord *, RecordCmp> > AccessCount; 		// For access frequency

	extern std::map<Function *, map<int, int64_t> > StackLayout;		// For stack layout

	bool AccessAnalysis(llvm::MachineFunction &mf);
	bool PackStack(MachineFunction &mf, int64_t &Offset, RegScavenger *RS, int min, int max, SmallSet<int, 16> &);
	int CacheOffset(MachineFunction &mf, std::map<int, CCacheBlock *> &hAlloc, int nOffset, int nPrev);
	int UpdateGraph(std::map<int, std::map<int, double> > &graph, std::map<int, std::set<int> > &removedEdge, CCacheBlock *pBlock, int nIndex);
	int AssignOffset(std::list<CCacheBlock *> &Block_list, int nOffset, int nOriOff, MachineFunction &mf, bool StackGrowsDown);
	int DumpGraph(std::map<int, std::map<int, double> > &graph);

	int UpdateEdge(std::map<int, std::map<int, AccessEdge *> > &AccEdge, int nFirst, int nSec, bool bFirst, bool bSec, double dFreq);
    double GetFrequency(const MachineBasicBlock *mbb,const Function *fn);
//}

#endif
