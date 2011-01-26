#include "llvm/Function.h"
#include "llvm/CodeGen/SPM_Alloc_FCFS.h"
#include "llvm/CodeGen/SPM.h"
#include "llvm/Support/raw_ostream.h"
#undef DEBUG_TYPE
#define DEBUG_TYPE "spm"
using namespace std;
using namespace llvm;
using namespace SPM;

namespace SPM
{
	map<enum MemoryKind, CMemory *> g_PwaMemory;
	map<enum MemoryKind, CMemory *> g_FcfsMemory;
	map<const MachineFunction *, list<CData *> > g_hDataList;
	int g_nNumOfMemory = 0;	
}
std::string SPM::CData::Name()
{ 
	//std::string szFunc = m_pMF->getFunction()->getName();
	char digits[6];
	sprintf(digits, "%d", m_nReg);
	return m_szFunc + "#" + digits;
}

double SPM::CData::getBenefit()
{ 
	assert( !m_MemoryCostSet.empty() );
	
	m_dBenefit = 0.0;
	set<struct MemoryCost>::const_iterator I = m_MemoryCostSet.begin(), J = ++ I;
	if( J == m_MemoryCostSet.end() )
		m_dBenefit = 0.0;
	else 
		m_dBenefit = I->m_dCost - J->m_dCost;
	return m_dBenefit;
}
void SPM::CDataClass::dump()
{
	std::list<CData *>::iterator I = this->m_Dataset.begin(), E = m_Dataset.end();
	for(; I != E; ++I )
		llvm::outs() << (*I)->Name() << "\t";
	llvm::outs() << "\n";
}

unsigned int SPM::CAllocNode::addData(SPM::CData *data)
{
	unsigned int nDiff = 0;
	if( m_nSize < data->size())
	{
		nDiff = data->size() - m_nSize;
		m_nSize = data->size();		
	}
	m_Dataset.push_back(data);
	m_FunctionSet.insert(data->getFunction() );
	//m_dTotalCost += data->m_hMemoryCost[m_kind];
	if(m_type == AT_FCFS)
		g_FcfsMemory[m_kind]->m_nReserved -= nDiff;
	else
		g_PwaMemory[m_kind]->m_nReserved -= nDiff;
	return nDiff;
}

unsigned int SPM::CAllocNode::addDataset(SPM::CDataClass *dataClass)
{
	unsigned int nDiff = 0;
	std::list<CData *>::iterator I = dataClass->m_Dataset.begin(), E = dataClass->m_Dataset.end();
	for(; I != E; ++I)
	{
		unsigned int nD = addData(*I);
		if( nD > nDiff)
			nDiff = nD;
	}	
	return nDiff;
}
int SPM::SpmAllocator::run(const llvm::MachineFunction *MF, const AccessFrequency *af, const InterfereGraph *ig)
{
	GetConfig();
	GetAccess(af);
	GetInterfere(ig);
	ComputeDataCost(MF);
	
	FCFS fcfs;
	fcfs.Allocator(MF);
	
	PWA pwa;
	pwa.Allocator(MF);
	return 0;
}


int SPM::SpmAllocator::GetConfig()
{
	if( g_nNumOfMemory == 0 )
	{
		/*g_nNumOfMemory = 3;
		g_FcfsMemory[SPM::PCM] = new CMemory( SPM::PCM, 2.5, 7.5, 16);
		g_FcfsMemory[SPM::SRAM] = new CMemory (SPM::SRAM, 1, 1, 16);
		g_FcfsMemory[SPM::DRAM] = new CMemory (SPM::DRAM, 50, 50, 512 << 10);	
		
		g_PwaMemory[SPM::PCM] = new CMemory( SPM::PCM, 2.5, 7.5, 16);
		g_PwaMemory[SPM::SRAM] = new CMemory (SPM::SRAM, 1, 1, 16);
		g_PwaMemory[SPM::DRAM] = new CMemory (SPM::DRAM, 50, 50, 512 << 10);*/	
		
		std::ifstream fin;
		fin.open("/home/qali/Develop/Output/memory.config");
		
		int nNumber = 0;
		fin >> nNumber;
		g_nNumOfMemory = nNumber;
		while( nNumber > 0)
		{
			-- nNumber;
			std::string szKind;
			enum MemoryKind kind;
			double dRead = 0.0, dWrite = 0.0;
			std::string szSize;
			unsigned int nSize = 0;
			
			fin >> szKind >> dRead >> dWrite >> szSize;
			if( szKind == "SRAM" )
				kind = SPM::SRAM;
			else if( szKind == "PCM" )
				kind = SPM::PCM;
			else if( szKind == "DRAM" )
				kind = SPM::DRAM;
			else
				llvm::outs() << "Eror: Unrecognized memory kind in configure file: " << szKind << "\n";
			unsigned int nLenth = szSize.size();
			-- nLenth;
			unsigned int nOrder = 0;
			bool bID = true;
			if(szSize.find_last_of('M') == nLenth )
				nOrder = 20;
			else if( szSize.find_last_of('K') == nLenth)
				nOrder = 10;
			else if( szSize.find_last_of('B') == nLenth )
				nOrder = 0;
			else 
				bID = false;
			
			if( bID )
				szSize = szSize.substr(0, nLenth);
			nSize = atoi(szSize.c_str());
			nSize <<= nOrder;
			g_FcfsMemory[kind] = new CMemory(kind, dRead, dWrite, nSize);
			g_PwaMemory[kind] = new CMemory(kind, dRead, dWrite, nSize);
		}		
		fin.close();
	}
	return 0;
}

// Get access info
int SPM::SpmAllocator::GetAccess(const AccessFrequency *af)
{
	for( DenseMap<int,double>::const_iterator DMI = af->m_RegReadMap.begin(), DME = af->m_RegReadMap.end();
		DMI != DME; ++DMI)
    {
		CData *pData = new CData();
		pData->setFunction( af->MF->getFunction()->getName());
		pData->setRead(DMI->second);
		pData->setRegNum(DMI->first);
		
		DenseMap<int, double>::const_iterator WI = af->m_RegWriteMap.find(DMI->first); 
		if( WI == af->m_RegWriteMap.end() )
			pData->setWrite(0);
		else
			pData->setWrite(WI->second);
		g_hDataList[af->MF].push_back(pData);
		t_hData[DMI->first] = pData;
	}
	return 0;
}

// Get interfere info
int SPM::SpmAllocator::GetInterfere(const InterfereGraph *ig)
{
	map<int, set<int> >::const_iterator I = ig->m_IGraph.begin(), E = ig->m_IGraph.end();
	for(; I != E; ++I )
	{
		set<int>::const_iterator II = I->second.begin(), EE = I->second.end();
		for(; II != EE; ++II)
			t_hData[I->first]->m_InterfereSet.insert(t_hData[*II]);
	}
	return 0;
}

int SPM::SpmAllocator::ComputeDataCost(const MachineFunction *MF)
{
	std::list<CData *> &DataList = g_hDataList[MF];
	std::list<CData *>::iterator I = DataList.begin(), E = DataList.end();
	for(; I != E; ++I)
	{
		CData *pData = *I;
		map<enum MemoryKind, CMemory *>::const_iterator IM = g_FcfsMemory.begin(), EM = g_FcfsMemory.end();
		for(; IM != EM; ++IM)
		{
			struct MemoryCost Cost;
			Cost.m_kind = IM->first;
			Cost.m_dCost = IM->second->m_dCostOfRead * pData->getRead() + IM->second->m_dCostOfWrite * pData->getWrite();
			pData->m_MemoryCostSet.insert(Cost);
			pData->m_hMemoryCost[Cost.m_kind] = Cost.m_dCost;
		}
	}
	return 0;
}

/*int SPM::CDataClass::updateSize()
{
	if(m_Dataset.empty() )
		return -1;
		
	m_nSize = m_Dataset.front()->size();
	std::list<CData *>::iterator I = m_Dataset.begin(), E = m_Dataset.end();
	for(; I != E; ++I)
	{
		if((*I)->size() > nBig )
			m_nSize = (*I)->size();
	}
	return 0;
}*/

bool SPM::CDataClass::testClass(SPM::CData *data)
{
	std::list<CData *>::iterator I = m_Dataset.begin(), E = m_Dataset.end();
	for(; I != E; ++I)
		if(data->m_InterfereSet.find(*I) != data->m_InterfereSet.end())
			return false;
	return true;
}

double SPM::CDataClass::getBenefit()
{ 
	assert( !m_MemoryCostSet.empty() );
		
	set<struct MemoryCost>::const_iterator I = m_MemoryCostSet.begin(), J = ++ I;
	if( J == m_MemoryCostSet.end() )
		m_dBenefit = 0.0;
	else 
		m_dBenefit = J->m_dCost - I->m_dCost;
	return m_dBenefit;
}

string SPM::MemoryName(enum MemoryKind kind)
{
	string szName;
	switch (kind)
	{
	case SPM::SRAM: szName = "SRAM"; break;
	case SPM::PCM: szName = "PCM"; break;
	case SPM::DRAM: szName = "DRAM"; break;
	default:
		assert("not supported memory kind!");				
	}
	return szName;
}
void SPM::dumpMemory(std::map<enum MemoryKind, CMemory *> &memoryList, ostream &OS )
{
	std::map<enum MemoryKind, CMemory *>::iterator I = memoryList.begin(), E = memoryList.end();
	double dTotal = 0.0;
	for(; I != E; ++I)
	{
		double dCost = 0.0;
		SPM::CMemory *pMemory = I->second;
		OS << "^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^\n";
		OS << SPM::MemoryName(pMemory->m_kind) << ":\t\n";		
		OS << "Cost:\t" << pMemory->m_dCostOfRead << "(per read)\t" << pMemory->m_dCostOfWrite << "(per write)\t\t";
		
		OS << "Used:\t" << pMemory->m_nSize - pMemory->m_nReserved << "/" << pMemory->m_nSize << ",\t";		
		OS << (pMemory->m_nSize - pMemory->m_nReserved)/(double)pMemory->m_nSize << "\n";
		
		DEBUG(OS << "Data allocation:\n");	
		list<SPM::CAllocNode *>::iterator IA = pMemory->m_AllocList.begin(), EA = pMemory->m_AllocList.end();
		unsigned int nOffset = 0;
		for(; IA != EA; ++IA)
		{
			SPM::CAllocNode *alloc = *IA;
			std::list<CData *>::iterator ID = alloc->m_Dataset.begin(), ED = alloc->m_Dataset.end();
			for(; ID != ED; ++ID)
			{
				DEBUG(OS << (*ID)->Name() << ":\t");
				map<SPM::MemoryKind, double>::iterator IMC = (*ID)->m_hMemoryCost.begin(), EMC = (*ID)->m_hMemoryCost.end();
				for(; IMC != EMC; ++IMC)
				{
					DEBUG(OS << SPM::MemoryName(IMC->first) << "(" << IMC->second << ")\t");
					if( IMC->first == pMemory->m_kind )
						dCost += IMC->second;
				}
				DEBUG(OS << "\tAddr:\t" << nOffset << "---" << nOffset + (*ID)->size() - 1 << "\n");	
			}
			nOffset += alloc->size();
		}
		OS << "\nCost in total for " << SPM::MemoryName(pMemory->m_kind) << ":\t" << dCost << "\n";
		dTotal += dCost;
	}
	OS << "\nCost in total for this algorithm: \t" << dTotal << "\n\n";
}

void SPM::dumpDatasetList(std::list<CDataClass *> &dclist)
{
	std::list<CDataClass *>::iterator I = dclist.begin(), E = dclist.end();
	for(; I != E ; ++I )
		(*I)->dump();
}