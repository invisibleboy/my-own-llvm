#ifndef LLVM_SPMALLOC_H
#define LLVM_SPMALLOC_H

#include "llvm/CodeGen/SPM.h"

using namespace llvm;
using namespace std;
namespace SPM
{
	// First come first servise heuristics
	class FcfsAlloc
	{
	public:
		FcfsAlloc() { }; 
		
		int Allocator( );       
		int run();
		
		//int Reset();

	private:
		int ComputeDataCost();
		int Allocate(CData *);

	private:
		//int m_nNumOfVariable;
		//std::list<CData *> &m_DataList;
		
	private:
		// intermediate data structure
		// std::map<int, CData *> t_hData;      // mapping from the nReg to Data
	};	
}
#endif
