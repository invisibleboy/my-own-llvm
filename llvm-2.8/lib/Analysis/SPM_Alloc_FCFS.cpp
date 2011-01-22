#include "llvm/Analysis/SPM_Alloc_FCFS.h"

using namespace SPM;

int FCFS::GetInput()
{
	if( g_nNumOfMemory == 0 )
	{
		g_nNumOfMemory = 3;
		g_MemoryList.push_back( new CMemory( SPM::PCM, 2.5, 7.5, 16));
		g_MemoryList.push_back( new CMemory (SPM::SRAM, 1, 1, 16) );
		g_MemoryList.push_back( new CMemory (SPM::DRAM, 50, 50, 512 << 20));	
	}
	
	// Get access info
/*	DenseMap<int, unsigned int>::const_iterator I = AccessFrequency::ms_instance->m_RegReadMap.begin();
	for( E = AccessFrequency::ms_instance->m_RegReadMap.end(); I != E; ++I )
	{
		int nRead = I->second;
		int nWrite = AccessFrequency::ms_instance->m_RegWriteMap.find( I->first )->second;
		CData *pData = new CData(I->first, nRead, nWrite, )
	}*/
	return 0;
}