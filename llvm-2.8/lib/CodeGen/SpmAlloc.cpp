#include "llvm/CodeGen/AccessFrequency.h"
#include "llvm/CodeGen/interferegraph.h"
#include "llvm/CodeGen/SpmAlloc.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/Support/raw_ostream.h"
#undef DEBUG_TYPE
#define DEBUG_TYPE "spm"
using namespace SPM;
using namespace llvm;
using namespace std;

extern std::map<enum MemoryKind, CMemory *> g_hMemory;
extern std::map<const llvm::MachineFunction *, list<CData *> > g_hDataList;
extern std::list<CData *> g_DataList;
extern std::map<std::string, std::set<std::string> > g_hFuncCall;

int XueAlloc::Allocator()
{
	std::list<CData *> &dataList = g_DataList;
	if( dataList.empty() )
		return 0;
	
	// The main process
	bool bSpilled = false;
	int nTotalSpilled = 0;
	
	for( int nKind = SPM::SRAM; nKind <= SPM::DRAM; ++ nKind)
	{					
		std::list<CData *>  data_list;
		std::list<CData *>::iterator d_lp = dataList.begin(), d_le = dataList.end();	
		for(d_lp = dataList.begin(); d_lp != d_le; ++ d_lp)
		{
			SPM::MemoryKind kind = (*d_lp)->m_MemoryCostList.front()->m_kind;
			if( kind == nKind )
				data_list.push_back(*d_lp);
		}
		
		// 1. Get the data-subgraph for each memory unit
		bSpilled = false;	
		// temporary data structures
		std::list<CData *> data_stack;					// data stack
		std::map<CData *, unsigned int> hDegree;       // degree for each data
		std::map<CData *, std::set<CData *> > hInter;  		
			
		// 2.1.1 compute the inters (only nodes in the same memory uint are considered )
		std::list<CData *>::iterator dl_I = data_list.begin(), dl_E = data_list.end();
		for(; dl_I != dl_E; ++ dl_I)
		{
			CData *pData = *dl_I;
			std::list<CData *>::iterator dl1_I = data_list.begin(), dl1_E = data_list.end();
			for(; dl1_I != dl1_E; ++ dl1_I)
			{
				CData *pData1 = *dl1_I;
				if( pData->IsInterfere(pData1) )
					hInter[pData].insert(pData1 );
			}
		}
		// 2.1.2 compute the degree for each data
		dl_I = data_list.begin();
		dl_E = data_list.end();
		for(; dl_I != dl_E; ++ dl_I)
		{
			unsigned int nDegree = 0;		
			nDegree = hInter[*dl_I].size();
			hDegree[*dl_I] = nDegree;
		}
			
		// 2.2 handle each subgraph in descend order
		
		for( d_lp = dataList.begin(); d_lp != d_le; ++ d_lp)
		{
			int nSpilled = 0;		
			
			SPM::MemoryKind kind = (SPM::MemoryKind)nKind;
			CMemory *memory = g_FcfsMemory[kind];
			unsigned int nColor = memory->m_nSize;			
			
			
			// 2.1.3 simplify and potential spill
			// 2.1.3.1 simplify
			while( !data_list.empty() )
			{					
				CData *pData = data_list.front();
				
				unsigned int nDegree = hDegree[pData];
				if( nDegree <= nColor )	// if colorable, push into the stack
				{
					data_stack.push_front(pData);
					std::set<CData *>::iterator ds_I = hInter[pData].begin(), ds_E = hInter[pData].end();
					for(; ds_I != ds_E; ++ ds_I)
					{
						-- hDegree[*ds_I];					// revise the subgraph by reducing the degree							
					}
					data_list.pop_front();
				}
				else 	// potentially spill a data into the stack
				{
					std::list<CData *>::iterator spill_p = GetSpilled(data_list, hDegree);
					data_stack.push_front(*spill_p);
					std::set<CData *>::iterator ds_I = hInter[*spill_p].begin(), ds_E = hInter[(*spill_p)].end();
					for(; ds_I != ds_E; ++ ds_I)
					{
						-- hDegree[*ds_I];					// revise the subgraph by reducing the degree ?unit size of degree, 1 or 4							
					}
					data_list.erase(spill_p);
				}				
			}	
			
			// 2.1.3.2 select and actual spill
			std::map<unsigned int, set<CData *> > &hColor = memory->m_ColorList;
			while( !data_stack.empty() )
			{
				CData *pData = data_stack.front();
				data_stack.pop_front();	
				bool bSucceed = false;
				for( int i = 0; i < nColor; ++ i)
				{
					// Find a new color
					if( hColor.find(i) == hColor.end() || hColor[i].empty())
					{
						hColor[i].insert(pData);
						bSucceed = true;
						break;
					}
					// Compare with used colors
					else
					{
						assert(!hColor[i].empty() );
						set<CData *> &data_set = hColor[i];
						set<CData *>::iterator ds_I = data_set.begin(), ds_E = data_set.end();
						for(; ds_I != ds_E; ++ ds_I)
							if( hInter[pData].find(*ds_I) != hInter[pData].end() )
								break;
						// If no interfered node with this color, it is fine
						if(ds_I == ds_E )
						{
							hColor[i].insert(pData);
							bSucceed = true;
							break;
						}
					}								
				}
				
				// If fail, need actual spilling
				if( !bSucceed )
				{
					pData->m_MemoryCostList.pop_front();
					bSpilled = true;
					++ nSpilled;					
				}			
			} // end for all elements of the stack			
			
			nTotalSpilled += nSpilled;
			errs() << "\nSpilled " << nSpilled << " times in " << MemoryName(kind) << "\n";
		} // end for each subgraph
		
	}while (bSpilled);
	errs() << "\nSpilled " << nTotalSpilled << " times in total!\n";
	
	return 0;
}

int MpcAlloc::Allocator()
{
	list<CData *> &dataList = g_DataList;
	if( dataList.empty() )
		return 0;
	
	// The main process
	bool bSpilled = false;
	int nTotalSpilled = 0;
	do
	{					
		// 1. Get the data-subgraph for each memory unit
		bSpilled = false;
		SetSubgraph(dataList);
		
		std::list<SPM::MemoryKind> subgraph_list;
		std::map<SPM::MemoryKind, std::set<CData *> >::iterator g_I = m_hSubgraph.begin(), g_E = m_hSubgraph.end();
		std::list<SPM::MemoryKind>::iterator m_I = subgraph_list.begin(), m_E = subgraph_list.end();
		
		// 2.0 sort the memory units  by cost in descend order
		for( ; g_I != g_E; ++ g_I)
		{
			unsigned int nSize1 = m_hSubgraph[g_I->first].size();	
			m_I = subgraph_list.begin();
			m_E = subgraph_list.end();
			for(; m_I != m_E; ++ m_I)
			{
				unsigned int nSize2 = m_hSubgraph[*m_I].size();
				if( nSize1 > nSize2 )
					break;
			}
			subgraph_list.insert(m_I, g_I->first);
		}
		
		// 2.1 handle each subgraph in descend order
		for( m_I = subgraph_list.begin(); m_I != m_E; ++ m_I)
		{
			int nSpilled = 0;
			std::list<CData *>  data_list;
			data_list.insert(data_list.begin(), m_hSubgraph[*m_I].begin(), m_hSubgraph[*m_I].end() );
			
			// temporary data structures
			std::list<CData *> data_stack;					// data stack
			std::map<CData *, unsigned int> hDegree;       // degree for each data
			std::map<CData *, std::set<CData *> > hInter;  
			
			CMemory *memory = g_FcfsMemory[*m_I];
			unsigned int nColor = memory->m_nSize;
			
			
			// 2.1.1 compute the inters (only nodes in the same memory uint are considered )
			std::list<CData *>::iterator dl_I = data_list.begin(), dl_E = data_list.end();
			for(; dl_I != dl_E; ++ dl_I)
			{
				CData *pData = *dl_I;
				std::list<CData *>::iterator dl1_I = data_list.begin(), dl1_E = data_list.end();
				for(; dl1_I != dl1_E; ++ dl1_I)
				{
					CData *pData1 = *dl1_I;
					if( pData->IsInterfere(pData1) )
						hInter[pData].insert(pData1 );
				}
			}
			
			// 2.1.2 compute the degree for each data
			dl_I = data_list.begin();
			dl_E = data_list.end();
			for(; dl_I != dl_E; ++ dl_I)
			{
				unsigned int nDegree = 0;
				/*std::set<CData *>::iterator ds_I = (*dl_I)->m_InterfereSet.begin(), ds_E = (*dl_I)->m_InterfereSet.end();
				for(; ds_I != ds_E; ++ ds_I)
					nDegree += (*ds_I)->size();*/
				nDegree = hInter[*dl_I].size();
				hDegree[*dl_I] = nDegree;
			}
			// 2.1.3 simplify and potential spill
			// 2.1.3.1 simplify
			while( !data_list.empty() )
			{					
				CData *pData = data_list.front();
				
				unsigned int nDegree = hDegree[pData];
				if( nDegree <= nColor )	// if colorable, push into the stack
				{
					data_stack.push_front(pData);
					std::set<CData *>::iterator ds_I = hInter[pData].begin(), ds_E = hInter[pData].end();
					for(; ds_I != ds_E; ++ ds_I)
					{
						-- hDegree[*ds_I];					// revise the subgraph by reducing the degree							
					}
					data_list.pop_front();
				}
				else 	// potentially spill a data into the stack
				{
					std::list<CData *>::iterator spill_p = GetSpilled(data_list, hDegree);
					data_stack.push_front(*spill_p);
					std::set<CData *>::iterator ds_I = hInter[*spill_p].begin(), ds_E = hInter[(*spill_p)].end();
					for(; ds_I != ds_E; ++ ds_I)
					{
						-- hDegree[*ds_I];					// revise the subgraph by reducing the degree ?unit size of degree, 1 or 4							
					}
					data_list.erase(spill_p);
				}				
			}	
			
			// 2.1.3.2 select and actual spill
			std::map<unsigned int, set<CData *> > &hColor = memory->m_ColorList;
			while( !data_stack.empty() )
			{
				CData *pData = data_stack.front();
				data_stack.pop_front();	
				bool bSucceed = false;
				for( int i = 0; i < nColor; ++ i)
				{
					// Find a new color
					if( hColor.find(i) == hColor.end() || hColor[i].empty())
					{
						hColor[i].insert(pData);
						bSucceed = true;
						break;
					}
					// Compare with used colors
					else
					{
						assert(!hColor[i].empty() );
						set<CData *> &data_set = hColor[i];
						set<CData *>::iterator ds_I = data_set.begin(), ds_E = data_set.end();
						for(; ds_I != ds_E; ++ ds_I)
							if( hInter[pData].find(*ds_I) != hInter[pData].end() )
								break;
						// If no interfered node with this color, it is fine
						if(ds_I == ds_E )
						{
							hColor[i].insert(pData);
							bSucceed = true;
							break;
						}
					}								
				}
				
				// If fail, need actual spilling
				if( !bSucceed )
				{
					pData->m_MemoryCostList.pop_front();
					bSpilled = true;
					++ nSpilled;					
				}			
			} // end for all elements of the stack
			
			// 2.1.4 clear the allocation
			if( bSpilled )
				hColor.clear();
			nTotalSpilled += nSpilled;
			errs() << "\nSpilled " << nSpilled << " times in " << MemoryName(*m_I) << "\n";
		} // end for each subgraph
		
	}while (bSpilled);
	errs() << "\nSpilled " << nTotalSpilled << " times in total!\n";
	
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
		DEBUG(llvm::errs() << (*pCandidate)->Name() << " wins the most benefit: " << (*pCandidate)->getBenefit() << " in " << (*(*pCandidate)->m_MemoryCostList.begin())->m_kind << "\n");
		DEBUG((*pCandidate)->dumpCostSet());
		// Allocate this most beneficial candidate
		int nRet = Allocate(*pCandidate);
		if( nRet == 0 )
			m_DataList.erase(pCandidate);
		else if( (*pCandidate)->m_MemoryCostList.empty() )
		{
			bSucceed = false;
			llvm::outs() << "Error: Failed to allocate " << (*pCandidate)->Name() << " in FCFS!\n";
		}
	}
	
}

int FcfsAlloc::Allocate(SPM::CData *pData)
{
	int nRet = 0;
	
	std::string szFunc = pData->getFunction();
	MemoryCost *pCost = *(pData->m_MemoryCostList.begin());
	enum MemoryKind kind = pCost->m_kind;
	CMemory *memory= g_FcfsMemory[kind];
	std::list<CAllocNode *> &allocList = memory->m_AllocList;
	std::list<CAllocNode *>::iterator AI = allocList.begin(), AE = allocList.end();
	bool bFound = false;
	for(; AI != AE; ++AI )
	{	
		bFound = true;
		std::list<CData *>::iterator DI = (*AI)->m_Dataset.begin(), DE = (*AI)->m_Dataset.end();
		for(; DI != DE; ++DI )
			if(pData->IsInterfere(*DI) )
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
	
	delete pCost;
	pData->m_MemoryCostList.pop_front();   
	//pData->updateBenefit();
	
	return nRet;
}

std::list<CData *>::iterator MpcAlloc::GetSpilled( std::list<CData *> &data_list, std::map<CData *, unsigned int> &hDegree)
{
	std::list<CData *>::iterator rec_p = data_list.begin(), dl_I = data_list.begin(), dl_E = data_list.end();
	double dRec = 0.0;
	for(; dl_I != dl_E; ++ dl_I)
	{
		CData *pData = *dl_I;
		unsigned int nDegree = hDegree[*dl_I];
		double dFactor = pData->getBenefit()/nDegree;	
		if( dl_I == rec_p || dFactor < dRec  )		
		{
			rec_p = dl_I;
			dRec = dFactor;
		}
	}
	return rec_p;
}

void MpcAlloc::SetSubgraph(std::list<CData *> &data_list )
{
	m_hSubgraph.clear();
	std::list<CData *>::iterator dl_I = data_list.begin(), dl_E = data_list.end();
	for(; dl_I != dl_E; ++ dl_I)
	{
		CData *pData = *dl_I;
		SPM::MemoryKind kind = pData->m_MemoryCostList.front()->m_kind;
		
		if( kind != SPM::DRAM)
			m_hSubgraph[kind].insert( pData );
		else
		{
			CMemory *memory = g_FcfsMemory[kind];
			memory->m_ColorList[0].insert(pData);
		}
	}
	
	std::map<SPM::MemoryKind, std::set<CData *> >::iterator m_p = m_hSubgraph.begin(), m_e = m_hSubgraph.end();
	for(; m_p != m_e; ++ m_p)
	{
		errs() << "In memory " << m_p->first << ":\t" << m_p->second.size() << " variables!\n";
	}
}

std::list<CData *>::iterator XueAlloc::GetSpilled( std::list<CData *> &data_list, std::map<CData *, unsigned int> &hDegree)
{
	std::list<CData *>::iterator rec_p = data_list.begin(), dl_I = data_list.begin(), dl_E = data_list.end();
	double dRec = 0.0;
	for(; dl_I != dl_E; ++ dl_I)
	{
		CData *pData = *dl_I;
		unsigned int nDegree = hDegree[*dl_I];
		double dFactor = (pData->getRead()+pData->getWrite())/nDegree;	
		if( dl_I == rec_p || dFactor < dRec  )		
		{
			rec_p = dl_I;
			dRec = dFactor;
		}
	}
	return rec_p;
}