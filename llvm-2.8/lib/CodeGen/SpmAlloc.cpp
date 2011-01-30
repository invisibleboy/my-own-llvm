#include "llvm/CodeGen/AccessFrequency.h"
#include "llvm/CodeGen/interferegraph.h"
#include "llvm/CodeGen/SpmAlloc.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/Support/raw_ostream.h"
#undef DEBUG_TYPE
#define DEBUG_TYPE "spm"
using namespace SPM;
using namespace llvm;

extern std::map<enum MemoryKind, CMemory *> g_hMemory;
extern std::map<const llvm::MachineFunction *, list<CData *> > g_hDataList;
extern std::list<CData *> g_DataList;

int FcfsAlloc::run()
{
	
	return 0;
}
int FcfsAlloc::Allocator()
{
	list<CData *> &m_DataList = g_DataList;
	if( m_DataList.empty() )
		return 0;
		
	bool bSucceed = true;	
	while( !m_DataList.empty() && bSucceed)
	{
		std::list<CData *>::iterator pCandidate = m_DataList.begin();
		// Find the most beneficial candidate		
		std::list<CData *>::iterator d_i = m_DataList.begin(), d_e = m_DataList.end();
		for( ; d_i != d_e; ++d_i)
		{
			if( (*d_i)->getBenefit() > (*pCandidate)->getBenefit())
				pCandidate = d_i;			
		}
		DEBUG(llvm::errs() << (*pCandidate)->Name() << " wins the most benefit: " << (*pCandidate)->getBenefit() << "\n");
		// Allocate this most beneficial candidate
		int nRet = Allocate(*pCandidate);
		if( nRet == 0 )
			m_DataList.erase(pCandidate);
		else if( (*pCandidate)->m_MemoryCostSet.empty() )
		{
			bSucceed = false;
			llvm::outs() << "Error: Failed to allocate " << (*pCandidate)->Name() << " in FCFS!\n";
		}
	}
	
}

int FcfsAlloc::Allocate(SPM::CData *pData)
{
	int nRet = 0;
	
	enum MemoryKind kind = pData->m_MemoryCostSet.begin()->m_kind;
	CMemory *memory= g_FcfsMemory[kind];
	std::list<CAllocNode *> &allocList = memory->m_AllocList;
	std::list<CAllocNode *>::iterator AI = allocList.begin(), AE = allocList.end();
	bool bFound = false;
	for(; AI != AE; ++AI )
	{
		// functions intefere
		if( *((*AI)->m_FunctionSet.begin()) != pData->getFunction() )
			continue;
		// live ranges interfere 
		std::list<CData *>::iterator DI = (*AI)->m_Dataset.begin(), DE = (*AI)->m_Dataset.end();
		bFound = true;
		for(; DI != DE; ++DI )
			if( pData->m_InterfereSet.find(*DI) != pData->m_InterfereSet.end())
			{
				bFound = false;
				break;
			}
		
		// Ok now
		if( bFound )
			break;
	}
	
	// The first allocation for this memory, or need new allocation
	if( !bFound )
	{
		if( memory->m_nReserved >= pData->size())
		{
			CAllocNode *allocNode = new CAllocNode(kind, SPM::AT_FCFS);
			allocNode->addData(pData);			
			allocList.push_back(allocNode);				
		}
		// This most memory cost could not be saved for lack of memory
		else
		{						    
			nRet = -1;;												
		}
	}
	else
	{
		CAllocNode *allocNode = *AI;
		if(allocNode->addData(pData) < 0 )
		{
			nRet = -1;
		}
	}
	
	pData->m_MemoryCostSet.erase(pData->m_MemoryCostSet.begin() );   
	//pData->updateBenefit();
	
	return nRet;
}
