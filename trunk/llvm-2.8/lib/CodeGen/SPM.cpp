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
extern std::map<std::string, std::set<std::string> > g_hFuncCall;
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
	std::map<std::string, std::set<std::string> > g_hFuncInter;
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
	assert( !m_MemoryCostList.empty() );
	
	m_dBenefit = 0.0;
	//std::set<MemoryCost *, MCCompare>::const_iterator I = m_MemoryCostSet.begin(), J = I;
	std::list<MemoryCost *>::const_iterator I = m_MemoryCostList.begin(), J = I;
	++ J;
	if( J == m_MemoryCostList.end() )
		m_dBenefit = 0.0;
	else 
	{
		MemoryCost *first = *I;
		MemoryCost *second = *J;
		m_dBenefit = second->m_dCost - first->m_dCost;
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
	}
	
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
	m_Dataset.push_back(data);
	m_FunctionSet.insert(data->getFunction() );
	if( nDiff > 0 )
		m_nSize = data->size();	
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
	 llvm::errs() << "Read Data file: " << g_DataList.size() << " amount\n" ;
	 fin.close();
	 
	 std::string sizeFile = BaseName + ".opt.size";
	 fin.open(sizeFile.c_str());
	 GetSize(fin);
	 llvm::errs() << "Read size file: " << g_nTotalSize << " amount\n" ;
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

	 dumpGlobalIG(module);
	 ComputeDataCost();
	 
	 //g_bBSA = false;
	//DEBUG(g_bBSA = true);
	 
	if( false )
	{
		FcfsAlloc fcfsAlloc;
		fcfsAlloc.Allocator();
	}
	else if(false)
	{
		MpcAlloc mpcAlloc;
		mpcAlloc.Allocator();
	}
	else
	{
		XueAlloc xueAlloc;
		xueAlloc.Allocator();
	}
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
		
		// remove unused functions
		std::string szMain = "main";
		int n = g_hFuncCall.size();
		int nn = g_hFuncCall[szMain].size();
		int nnn = g_hFuncCall["main"].size();
		if( szFunc != szMain && g_hFuncCall[szMain].find(szFunc) == g_hFuncCall[szMain].end())
			continue;
		
		szName = szName.substr(nFunc+1, nLen-nFunc-1);
		//DEBUG(if( szName.size() <= 3 ) llvm::errs() << szFunc << ":" << szName << "\n");
			
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
				assert(pData != NULL && ptmpData != NULL);
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
			MemoryCost *Cost = new MemoryCost();
			Cost->m_kind = IM->first;
			Cost->m_dCost = (IM->second->m_dCostOfRead * pData->getRead() + IM->second->m_dCostOfWrite * pData->getWrite());
			//pData->m_MemoryCostSet.insert(Cost);
			pData->m_hMemoryCost[Cost->m_kind] = Cost->m_dCost;
			
			/*std::list<MemoryCost *>::iterator m_p = pData->m_MemoryCostList.begin(), m_e = pData->m_MemoryCostList.end();
			for(; m_p != m_e; ++ m_p)
			{
				if(Cost->m_dCost < (*m_p)->m_dCost )
					break;
			}
			pData->m_MemoryCostList.insert(m_p, Cost);*/
			
			// sort it by SRAM < PCM < DRAM
			std::list<MemoryCost *>::iterator m_p = pData->m_MemoryCostList.begin(), m_e = pData->m_MemoryCostList.end();
			for(; m_p != m_e; ++ m_p)
			{
				if(Cost->m_kind < (*m_p)->m_kind )
					break;
			}
			pData->m_MemoryCostList.insert(m_p, Cost);
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
			MemoryCost *Cost = new MemoryCost();
			Cost->m_kind = IM->first;
			Cost->m_dCost = (IM->second->m_dCostOfRead * pData->getRead() + IM->second->m_dCostOfWrite * pData->getWrite());
			pData->m_MemoryCostSet.insert(Cost);
			pData->m_hMemoryCost[Cost->m_kind] = Cost->m_dCost;
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
		
	set<MemoryCost *, MCCompare>::const_iterator I = m_MemoryCostSet.begin(), J = ++ I;
	if( J == m_MemoryCostSet.end() )
		m_dBenefit = 0.0;
	else 
		m_dBenefit = (*J)->m_dCost - (*I)->m_dCost;
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

double SPM::dumpMpcMemory(std::map<enum MemoryKind, CMemory *> &memoryList, ostream &OS)
{
	std::map<enum MemoryKind, CMemory *>::iterator I = memoryList.begin(), E = memoryList.end();
	double dTotal = 0.0;
	double dTotalReads = 0.0;
	double dTotalWrites = 0.0;
	for(; I != E; ++I)
	{
		double dReads = 0.0;
		double dWrites = 0.0;
		double dCost = 0.0;
		SPM::CMemory *pMemory = I->second;
		OS << "^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^\n";
		OS << SPM::MemoryName(pMemory->m_kind) << ":\t\n";		
		OS << "Cost:\t" << pMemory->m_dCostOfRead << "(per read)\t" << pMemory->m_dCostOfWrite << "(per write)\t\t";
		
		OS << "Used:\t" << pMemory->m_ColorList.size() << "/" << pMemory->m_nSize << ",\t";		
		OS << pMemory->m_ColorList.size()/(double)pMemory->m_nSize << "\n";
		
		DEBUG(OS << "Data allocation:\n");	
		map<unsigned int, std::set<CData *> >::iterator i2c_p = pMemory->m_ColorList.begin(), i2c_e = pMemory->m_ColorList.end();
		for(; i2c_p != i2c_e; ++ i2c_p)
		{
			set<CData *>::iterator ds_p = i2c_p->second.begin(), ds_e = i2c_p->second.end();
			for(; ds_p != ds_e; ++ ds_p )
			{
				CData *pData = *ds_p;
				DEBUG(OS << pData->Name() << ":\t");
				map<SPM::MemoryKind, double>::iterator IMC = pData->m_hMemoryCost.begin(), EMC = pData->m_hMemoryCost.end();
				for(; IMC != EMC; ++IMC)
				{
					DEBUG(OS << SPM::MemoryName(IMC->first) << "(" << IMC->second << ")\t");
					if( IMC->first == pMemory->m_kind )
					{
						dReads += pData->getRead() ;
						dWrites += pData->getWrite() ; // don't consider data size
					}
					if( IMC->first == pMemory->m_kind )
						dCost += IMC->second;
				}
				DEBUG(OS << "\tAddr:\t" << i2c_p->first << "---" << i2c_p->first + pData->size() - 1 << "\n");	
			}
		}
		OS << "\nCost in total for " << SPM::MemoryName(pMemory->m_kind) << ":\t" << dCost << "\n";
		OS << "\nReads: " << dReads << "\tWrites: " << dWrites << "\n\n";
		dTotal += dCost;
		dTotalReads += dReads;
		dTotalWrites += dWrites;		
	}
	OS << "\nCost in total for this algorithm: \t" << dTotal << "\n";
	OS << "\nTotal Reads: " << dTotalReads << "\tTotal Writes: " << dTotalWrites << "\n\n";
	return dTotal;
}
double SPM::dumpMemory(std::map<enum MemoryKind, CMemory *> &memoryList, ostream &OS )
{
	std::map<enum MemoryKind, CMemory *>::iterator I = memoryList.begin(), E = memoryList.end();
	double dTotal = 0.0;
	double dTotalReads = 0.0;
	double dTotalWrites = 0.0;
	for(; I != E; ++I)
	{
		double dReads = 0.0;
		double dWrites = 0.0;
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
					{
						dReads += (*ID)->getRead() * (*ID)->size();
						dWrites += (*ID)->getWrite() * (*ID)->size();
					}
					if( IMC->first == pMemory->m_kind )
						dCost += IMC->second;
				}
				DEBUG(OS << "\tAddr:\t" << nOffset << "---" << nOffset + (*ID)->size() - 1 << "\n");	
			}
			nOffset += alloc->size();
		}
		OS << "\nCost in total for " << SPM::MemoryName(pMemory->m_kind) << ":\t" << dCost << "\n";
		OS << "\nReads: " << dReads << "\tWrites: " << dWrites << "\n\n";
		dTotal += dCost;
		dTotalReads += dReads;
		dTotalWrites += dWrites;
	}
	OS << "\nCost in total for this algorithm: \t" << dTotal << "\n";
	OS << "\nTotal Reads: " << dTotalReads << "\tTotal Writes: " << dTotalWrites << "\n\n";
	return dTotal;
}

void SPM::dumpDatasetList(std::list<CDataClass *> &dclist)
{
	std::list<CDataClass *>::iterator I = dclist.begin(), E = dclist.end();
	for(; I != E ; ++I )
		(*I)->dump();
}

int SPM::SpmAllocator::dumpGlobalIG(llvm::Module *mod)
{
	std::string fileName = mod->getModuleIdentifier() + ".gIG";	
	std::ofstream fout;
	fout.open(fileName.c_str(), std::ios_base::out);
	
	std::list<CData *> &DataList = g_DataList;
	std::list<CData *>::iterator I = DataList.begin(), E = DataList.end();
	int nCount = 0;
	for(; I != E; ++I)
	{
		const string &srcFunc = (*I)->getFunction();
		std::list<CData *>::iterator I1 = I, E1 = DataList.end();
		++I1;
		for(; I1 != E1; ++I1)
		{
			const string &dFunc = (*I1)->getFunction();
			// Function interfere
			if( g_hFuncCall[srcFunc].find(dFunc) != g_hFuncCall[srcFunc].end()
				|| g_hFuncCall[dFunc].find(srcFunc) != g_hFuncCall[dFunc].end() )
			{
				fout << srcFunc << "_reg" << (*I)->m_nReg << ",";
				fout << dFunc << "_reg" << (*I1)->m_nReg << " ";		
				++nCount;
				if( nCount % 5 == 0)
					fout << "\r\n";
			}
			// live range interfere
			else if((*I)->m_InterfereSet.find((*I1) ) != (*I)->m_InterfereSet.end()
				|| (*I1)->m_InterfereSet.find((*I) ) != (*I1)->m_InterfereSet.end())
			{
				fout << srcFunc << "_reg" << (*I)->m_nReg << ",";
				fout << dFunc << "_reg" << (*I1)->m_nReg << " ";
				++nCount;
				if( nCount % 5 == 0)
					fout << "\r\n";
			}			
		}
	}
	fout << "~\r\n";
	fout.close();
	return 0;
}


void SPM::dumpFunctionCall(ostream &OS)
{
	std::map<std::string, std::set<std::string> >::iterator f2f_p = g_hFuncCall.begin(), f2f_e = g_hFuncCall.end();
	for(; f2f_p != f2f_e; ++ f2f_p)
	{
		OS << f2f_p->first << ":\t";
		std::set<std::string>::iterator f_p = f2f_p->second.begin(), f_e = f2f_p->second.end();
		for(; f_p != f_e; ++ f_p)
			OS << *f_p << ", ";
		OS << "\n";
	}
}

void SPM::CData::dumpCostSet()
{
	std::list<MemoryCost *>::iterator MI = m_MemoryCostList.begin(), ME = m_MemoryCostList.end();
	for( ; MI != ME; ++ MI)
		llvm::outs() << (*MI)->m_kind << "," << (*MI)->m_dCost << '\t';
	llvm::outs() << "\n";
}

bool CData::IsInterfere(CData *right)
{
	if( this->m_InterfereSet.find(right) != this->m_InterfereSet.end()
		|| right->m_InterfereSet.find(this) != right->m_InterfereSet.end() )
		return true;
	
	std::string srcFunc = this->getFunction();
	std::string dFunc = right->getFunction();
	if( g_hFuncCall[srcFunc].find(dFunc) != g_hFuncCall[srcFunc].end()
		|| g_hFuncCall[dFunc].find(srcFunc) != g_hFuncCall[dFunc].end() )
		return true;
	return false;
}