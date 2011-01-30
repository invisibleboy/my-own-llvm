#include "llvm/Function.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/CodeGen/SPM_Alloc_FCFS.h"
#include "llvm/CodeGen/SpmAlloc.h"
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
	std::map<std::string, std::map<int, CData *> > g_DataMap;
	std::list<CData *> g_DataList; 
	int g_nNumOfMemory = 0;	
	int g_nNumOfVars = 0;
	int g_nNumOfFuncs = 0;
	int g_nTotalSize = 0;
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
	set<struct MemoryCost>::const_iterator I = m_MemoryCostSet.begin(), J = I;
	++ J;
	if( J == m_MemoryCostSet.end() )
		m_dBenefit = 0.0;
	else 
	{
		MemoryCost first = *I;
		MemoryCost second = *J;
		m_dBenefit = second.m_dCost - first.m_dCost;
	}
	return m_dBenefit;
}
void SPM::CDataClass::dump()
{
	std::list<CData *>::iterator I = this->m_Dataset.begin(), E = m_Dataset.end();
	for(; I != E; ++I )
		llvm::outs() << (*I)->Name() << "\t";
	llvm::outs() << "\n";
}

int SPM::CAllocNode::addData(SPM::CData *data)
{
	int nDiff = 0;
	if( m_nSize < data->size())
	{
		nDiff = data->size() - m_nSize;
		m_nSize = data->size();		
	}
	m_Dataset.push_back(data);
	m_FunctionSet.insert(data->getFunction() );
	//m_dTotalCost += data->m_hMemoryCost[m_kind];
	if(m_type == AT_FCFS)
	{		
		if(g_FcfsMemory[m_kind]->m_nReserved < nDiff)
			return -1;
		g_FcfsMemory[m_kind]->m_nReserved -= nDiff;
	}
	else
	{		
		if(g_PwaMemory[m_kind]->m_nReserved < nDiff)
			return -1;
		g_PwaMemory[m_kind]->m_nReserved -= nDiff;
	}
	return nDiff;
}

int SPM::CAllocNode::addDataset(SPM::CDataClass *dataClass)
{
	int nDiff = 0;
	std::list<CData *>::iterator I = dataClass->m_Dataset.begin(), E = dataClass->m_Dataset.end();
	for(; I != E; ++I)
	{
		int nD = addData(*I);
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

int SPM::SpmAllocator::run(Module *module)
{
	 GetConfig();
	 
	 ifstream fin;
	 std::string BaseName = module->getModuleIdentifier();
	 unsigned int nSuffix = BaseName.find_last_of(".bc");
	 BaseName = BaseName.substr(0, nSuffix);
	 
	 std::string varFile = BaseName + ".opt.var";
	 fin.open(varFile.c_str());
	 GetData(fin);
	 DEBUG(llvm::errs() << "Read Data file: " << g_DataList.size() << " amount\n" );
	 fin.close();
	 
	 std::string sizeFile = BaseName + ".opt.size";
	 fin.open(sizeFile.c_str());
	 GetSize(fin);
	 DEBUG(llvm::errs() << "Read size file: " << g_nTotalSize << " amount\n" );
	 fin.close();
	 
	 std::string readFile = BaseName + ".opt.read";
	 fin.open(readFile.c_str());
	 GetAccess(fin);
	 fin.close();
	 
	 std::string wFile = BaseName + ".opt.write";
	 fin.open(wFile.c_str());
	 GetAccess(fin, false);
	 fin.close();
	 
	 std::string igFile = BaseName + ".opt.ig";
	 fin.open(igFile.c_str());
	 GetInterfere(fin);
	 fin.close();

	 ComputeDataCost();
	 FcfsAlloc spmAlloc;
	 spmAlloc.Allocator();
	return 0;
}

int SPM::SpmAllocator::GetConfig()
{
	if( g_nNumOfMemory == 0 )
	{
		/*g_nNumOfMemory = 3;
		g_FcfsMemory[SPM::PCM] = new CMemory( SPM::PCM, 2.5, 7.5, 16);
		g_FcfsMemory[SPM::SRAM] = new CMemory (SPM::SRAM, 1, 1, 16);
		g_FcfsMemory[SPM::DRA
M] = new CMemory (SPM::DRAM, 50, 50, 512 << 10);	
		
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

int SPM::SpmAllocator::GetData(ifstream &datafile)
{
	std::string szName;
	std::string szFunc;
	std::string szReg;
	while( datafile >> szName)
	{
		unsigned int nLen = szName.size();
		if(szName.find_last_of(',') == nLen-1 )
			szName = szName.substr(0, nLen-1);
		unsigned int nFunc = szName.find("_reg");
		szFunc = szName.substr(0, nFunc);
		szName = szName.substr(nFunc+1, nLen-nFunc-1);
		DEBUG(if( szName.size() <= 3 ) llvm::errs() << szFunc << ":" << szName << "\n");
			
		szReg = szName.substr(3);
		unsigned int nReg = atoi(szReg.c_str());
		
		CData *pData = new CData();
		pData->setFunction( szFunc);
		pData->setRegNum(nReg);
		
		g_DataList.push_back(pData);
		g_DataMap[szFunc][nReg] = pData;
	}
	g_nNumOfFuncs = g_DataMap.size();
	g_nNumOfVars = g_DataList.size();
	return 0;
}

int SPM::SpmAllocator::GetSize(ifstream &sizeFile)
{
	std::list<CData *>::iterator DI = g_DataList.begin(), DE = g_DataList.end();
	
	std::string szName;
	while( sizeFile >> szName && DI != DE)
	{
		unsigned int nLen = szName.size();

		if(szName.find_last_of(',') == nLen-1 )
			szName = szName.substr(0, nLen-1);
			
		int nSize = atoi(szName.c_str());
		(*DI)->setSize(nSize);
		g_nTotalSize += nSize;
		++DI;
	}
	return 0;
}

int SPM::SpmAllocator::GetAccess(ifstream &rfile, bool bRead )
{
	std::list<CData *>::iterator DI = g_DataList.begin(), DE = g_DataList.end();
	
	std::string szName;
	while( rfile >> szName && DI != DE)
	{
		unsigned int nLen = szName.size();

		if(szName.find_last_of(',') == nLen-1 )
			szName = szName.substr(0, nLen-1);
			
		double dFreq = atof(szName.c_str());
		if( bRead)
			(*DI)->setRead(dFreq);
		else
			(*DI)->setWrite(dFreq);		
		++DI;
	}
	return 0;
}


int SPM::SpmAllocator::GetInterfere(ifstream &ig)
{
	
	std::string szFunc;	
	std::string szName;
	CData *pData = NULL;
	while( ig >> szName )
	{		
		unsigned int nLen = szName.size();
		if(szName.find_last_of(',') == nLen-1 )
		{
			while(true)
			{
				unsigned int nNum = szName.find_first_of(',');
				std::string szNum = szName.substr(0, nNum);
				int nReg = atoi(szNum.c_str());
				CData *ptmpData = g_DataMap[szFunc][nReg];
				pData->m_InterfereSet.insert(ptmpData);
				
				if( nNum + 1 == szName.size() )
					break;
				szName = szName.substr(nNum+1);
			}
		}
		else if( szName.find_last_of(':') == nLen-1)
		{
			unsigned int nFunc = szName.find("%reg");
			szFunc = szName.substr(0, nFunc);
			std::string szNum = szName.substr(nFunc+4);
			int nReg = atoi(szNum.c_str());
			pData = g_DataMap[szFunc][nReg];
		}
		else
		{
			continue;
		}
	}		
			
	return 0;
}

int SPM::SpmAllocator::ComputeDataCost()
{
	std::list<CData *> &DataList = g_DataList;
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
		pData->setSize(af->getRegSize(DMI->first));
		
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
double SPM::dumpMemory(std::map<enum MemoryKind, CMemory *> &memoryList, ostream &OS )
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
	return dTotal;
}

void SPM::dumpDatasetList(std::list<CDataClass *> &dclist)
{
	std::list<CDataClass *>::iterator I = dclist.begin(), E = dclist.end();
	for(; I != E ; ++I )
		(*I)->dump();
}