#ifndef LLVM_SPM_ALLOC_FCFS_H
#define LLVM_SPM_ALLOC_FCFS_H

#include <map>
#include <list>
#include <vector>
#include "llvm/CodeGen/SPM.h"

using namespace llvm;
using namespace std;
namespace SPM
{
	// First come first servise heuristics
	class FCFS
	{
	public:
		FCFS() {}; 
		
		int Allocator(const MachineFunction *);       
		
		//int Reset();

	private:
		int ComputeDataCost();
		int Allocate(CData *);

	private:
		//int m_nNumOfVariable;
		std::list<CData *> m_DataList;
		
	private:
		// intermediate data structure
		// std::map<int, CData *> t_hData;      // mapping from the nReg to Data
	};
	
	// partition-while-assign heuristics
	class PWA
	{
	public:
		int Allocator(const MachineFunction *);	
		
	private:
		int Coloring();
		int ComputeClassCost();
		int Allocate(CDataClass *);
	private:
		std::list<CData *> m_DataList;
		std::list<CDataClass *> m_DatasetList;
	};
}
#endif
