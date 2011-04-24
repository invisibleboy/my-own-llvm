#include "llvm/CodeGen/AccessFrequency.h"
#include "llvm/CodeGen/interferegraph.h"
#include "llvm/CodeGen/SPM_Alloc_FCFS.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/Support/raw_ostream.h"
#undef DEBUG_TYPE
#define DEBUG_TYPE "spm"
using namespace SPM;
using namespace llvm;

extern map<enum MemoryKind, CMemory *> g_hMemory;
extern map<const llvm::MachineFunction *, list<CData *> > g_hDataList;

int FCFS::Allocator(const llvm::MachineFunction *MF)
{
	m_DataList = g_hDataList[MF];
	
	if( m_DataList.empty() )
		return 0;
		
	ComputeDataCost();
	
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
	
	// If
	//Reset();
	
	if( !bSucceed )
	{		
		return -1;
	}
	return 0;
}


int FCFS::ComputeDataCost()
{
	// It has been done by the SpmAllocator now

	return 0;
}

int FCFS::Allocate(SPM::CData *pData)
{
	int nRet = 0;
	
	MemoryCost *pCost = *(pData->m_MemoryCostSet.begin());
	enum MemoryKind kind = pCost->m_kind;
	CMemory *memory= g_FcfsMemory[kind];
	std::list<CAllocNode *> &allocList = memory->m_AllocList;
	std::list<CAllocNode *>::iterator AI = allocList.begin(), AE = allocList.end();
	bool bFound = false;
	for(; AI != AE; ++AI )
	{
		// data of the two functions could not overlay
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
	
	pData->m_MemoryCostSet.erase(pCost );   
	//pData->updateBenefit();
	
	return nRet;
}

int PWA::Allocator(const llvm::MachineFunction *MF)
{
	m_DataList = g_hDataList[MF];
	
	if( m_DataList.empty() )
		return 0;
	
	(void) Coloring(); 			// Get the Dataset list
	(void) ComputeClassCost(); 		// Get the memorycost per dataset per memory unit
	
	bool bSucceed = true;	
	while( !m_DatasetList.empty() && bSucceed )
	{
		std::list<CDataClass *>::iterator pCandidate = m_DatasetList.begin();
		// Choose the most saving candidate
		std::list<CDataClass *>::iterator I = m_DatasetList.begin(), E = m_DatasetList.end();
		for(; I != E; ++I)
		{
			//(*I)->updateBenefit();	
			if( (*I)->getBenefit() > (*pCandidate)->getBenefit())
				pCandidate = I;
		}
		
		if( Allocate(*pCandidate) == 0 )
		{
			m_DatasetList.erase(pCandidate);
			//delete *pCandidate;
		}
		else if( (*pCandidate)->m_MemoryCostSet.empty())
		{
			llvm::outs() << "Error: Failed to allocate DataClass" << " in PWA!\n";
			(*pCandidate)->dump();
			bSucceed = false;
		}
	}
	
	if(!bSucceed)
	{	
		return -1;		
	}
	return 0;	
}

int PWA::Allocate(SPM::CDataClass *pDataClass)
{
	int nRet = 0;
	MemoryCost *pCost = *(pDataClass->m_MemoryCostSet.begin());
	enum MemoryKind kind = pCost->m_kind;
	CMemory *memory= g_PwaMemory[kind];
	
	if( memory->m_nReserved >= pDataClass->size())
	{
		SPM::CAllocNode *alloc = new CAllocNode(kind, SPM::AT_PWA);
		alloc->addDataset(pDataClass);
		memory->m_AllocList.push_back(alloc);
	}
	// This most memory cost could not be saved for lack of memory
	else
	{						    
		nRet = -1;												
	}
	
	delete pCost;
	pDataClass->m_MemoryCostSet.erase(pCost );   
	//pDataClass->updateBenefit();
	
	return nRet;
}
int PWA::Coloring()
{
	std::map<CData *, unsigned int> hDataDegree;
	std::list<CData *> dataStack;
	
	// The initial degrees for the data set
	std::list<CData *>::iterator I = m_DataList.begin(), E = m_DataList.end();
	for( ; I != E; ++I)
		hDataDegree[*I] = (*I)->m_InterfereSet.size();
		
	// Always choose the data with currently least degree to push the stack	
	while( !m_DataList.empty() )
	{
		std::list<CData *>::iterator pLeast = m_DataList.begin();
		I = pLeast; 
		I ++;
		for( ; I != E; ++I)
			if( hDataDegree[*I] < hDataDegree[*pLeast] )
				pLeast = I;
		
		// update the interfere graph		
		I = m_DataList.begin();
		for(; I != E; ++I )
			if( (*pLeast)->m_InterfereSet.find(*I) != (*pLeast)->m_InterfereSet.end() )
				--hDataDegree[*I];
				
		// push the chosen data into stack, and erase the handled data
		m_DataList.erase(pLeast);
		dataStack.push_front(*pLeast);
	}
	
	// Pop the data objects out of stack and color them into different classes	
	while( !dataStack.empty())
	{
		CData *pData = dataStack.front();
		dataStack.pop_front();
		
		std::list<CDataClass *>::iterator IC = m_DatasetList.begin(), EC = m_DatasetList.end();
		bool bFound = false;
		for(; IC != EC; ++IC)
		{
			SPM::CDataClass *pDataClass = *IC;			
			if( pDataClass->testClass(pData) )
			{
				pDataClass->add(pData);
				bFound = true;
				break;
			}			
		}
		
		// need a new class
		if( !bFound)
		{
			CDataClass *pDataClass = new CDataClass();
			pDataClass->add(pData);
			m_DatasetList.push_back(pDataClass);
		}		
	}
	return 0;
}

int PWA::ComputeClassCost()
{
	std::list<CDataClass *>::iterator IC = m_DatasetList.begin(), EC = m_DatasetList.end();
	for(; IC != EC; ++IC)
	{
		CDataClass *pDataClass = *IC;		
		map<enum MemoryKind, CMemory *>::const_iterator IM = g_PwaMemory.begin(), EM = g_PwaMemory.end();
		for(; IM != EM; ++IM)
		{
			double dCost = 0.0;
			std::list<CData *>::const_iterator I = pDataClass->m_Dataset.begin(), E = pDataClass->m_Dataset.end();
			for(; I != E; ++I )
			{
				CData *pData = *I;
				dCost += pData->m_hMemoryCost[IM->first];
			}
			MemoryCost *Cost = new MemoryCost();
			Cost->m_kind = IM->first;
			Cost->m_dCost = dCost;
			pDataClass->m_MemoryCostSet.insert(Cost);
		}
	}
	return 0;
}
