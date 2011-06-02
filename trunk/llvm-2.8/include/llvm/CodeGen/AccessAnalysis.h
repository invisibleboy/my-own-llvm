#ifndef __ACCESS_ANALYSIS_H_
#define __ACCESS_ANALYSIS_H_

#include <vector>
#include <map>
#include <set>
#include "llvm/ADT/SmallSet.h"
using namespace std;

#include "llvm/Function.h"
#include "llvm/CodeGen/MachineFunction.h"
using namespace llvm;

#define CASCH_LINE_SIZE	64
#define ASSOCIATIVITY	8
#define	CASCH_CAPACITY	(2 << 20)

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
	
	struct RecordCmp
	{
		
		bool operator () ( AccessRecord *first, AccessRecord *second) 
		{
			if( first->m_dCount < second->m_dCount)
				return true;
			return false;
		}
	};
	
	extern std::map<const Function *, std::set<AccessRecord *, RecordCmp> > AccessCount; 		// For access frequency
	
	extern std::map<Function *, map<int, int64_t> > StackLayout;		// For stack layout
	
	bool AccessAnalysis(llvm::MachineFunction &mf);
	bool PackStack(MachineFunction &mf, int64_t &Offset, RegScavenger *RS, int min, int max, SmallSet<int, 16> &);
	
//}

#endif
