#ifndef LLVM_SPM_ALLOC_FCFS_H
#define LLVM_SPM_ALLOC_FCFS_H

#include <map>
#include <list>
#include <vector>
#include "SPM.h"
/*#include "llvm/codegen/AccessFrequency.h"
#include "llvm/codegen/interferegraph.h"*/

using namespace std;
namespace SPM
{
	list<CMemory *> g_MemoryList;
	list<CData *> g_DataList;
	int g_nNumOfMemory = 0;

class FCFS
{
public:


public:
	FCFS() {}; 
	int GetConfig();
	int GetInput();
	int Allocate();

private:
	int ComputeCost();

private:
	int m_nNumOfVariable;

};
}
#endif
