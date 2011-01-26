#ifndef LLVM_SPM_H
#define LLVM_SPM_H

#include <map>
#include <list>
#include <vector>
#include <string>
#include <set>
#include <fstream>
#include "stdio.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h" 
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/AccessFrequency.h"
#include "llvm/CodeGen/interferegraph.h"
using namespace llvm;
using namespace std;

namespace SPM
{	
	
	class CData;
	class CDataClass;
	class CMemory;
	
	enum MemoryKind
	{
		PCM,
		SRAM,
		DRAM
	};
	
	enum AllocType
	{
		AT_FCFS,
		AT_PWA
	};

	class CAllocNode
	{
	public:
		CAllocNode(enum MemoryKind kind, enum AllocType type) { m_kind = kind; m_nSize = 0; m_type = type;}
				
		unsigned int addData(CData *data);
		unsigned int addDataset(CDataClass *dataClass);
		inline void setSize(unsigned int nSize ) { m_nSize = nSize; }
		inline unsigned int size() { return m_nSize; }
		
	private:		
		unsigned int m_nSize;
		enum MemoryKind m_kind;
		enum AllocType m_type;
	public:
		std::list<CData *> m_Dataset;
		std::set<string> m_FunctionSet;
	};
	
	class CMemory
	{
	public:
		enum MemoryKind  m_kind;
		double m_dCostOfRead;
		double m_dCostOfWrite;
		unsigned int m_nSize;
		unsigned int m_nReserved;
		double m_dTotalCost;
		std::list<CAllocNode *> m_AllocList;

	public:
		CMemory( enum MemoryKind kind, double costOfRead, double costOfWrite, unsigned int nSize)
		{
			m_kind = kind;
			m_dCostOfRead = costOfRead;
			m_dCostOfWrite = costOfWrite;
			m_nSize = nSize;
			m_nReserved = nSize;
			m_dTotalCost = 0.0;
		}		
	};

	struct MemoryCost
	{
		enum MemoryKind m_kind;
		double m_dCost;

		bool operator < (const struct MemoryCost &mc) const
		{
			return m_dCost < mc.m_dCost;
		}
	};

	class CData
	{
	private:
		string m_szFunc;
		double m_dReads;
		double m_dWrites;
		unsigned int m_nSize;
		unsigned int m_nReg;
		double m_dBenefit;
		
	public:
		std::set<CData *> m_InterfereSet;
		std::set<struct MemoryCost> m_MemoryCostSet;
		std::map<enum MemoryKind, double> m_hMemoryCost;

	public:
		CData()
		{			
			m_dReads = 0.0;
			m_dWrites = 0.0;
			m_nSize = 1;
		}
/*		CData(llvm::MachineFunction *pMF, double dReads, double dWrites, unsigned int nReg, unsigned int nSize = 1)
		{
			m_pMF = pMF;
			m_dReads = dReads;
			m_dWrites = dWrites;
			m_nSize = nSize;
			m_nReg = nReg;
		}*/

		inline void setRead(double dRead) { m_dReads = dRead; }
		inline void setWrite(double dWrite) { m_dWrites = dWrite; }
		inline void setFunction(string szFunc) { m_szFunc = szFunc; }
		inline void setSize(double nSize) { m_nSize = nSize; }
		inline void setRegNum(unsigned int nReg) { m_nReg = nReg; }
		//void updateBenefit(); 		
		
		inline double getRead() { return m_dReads; }
		inline double getWrite() {return m_dWrites; }		
		inline string getFunction() { return m_szFunc; }
		inline unsigned int size() { return m_nSize; }
		
		string Name(); 		
		double getBenefit() ;
	};
	
	class CDataClass
	{
	public:
		CDataClass() { m_nSize = 0;}		
		
	public:
		inline unsigned int size() { return m_nSize; }
		inline void add(CData *data) 
		{ 
			m_Dataset.push_back(data); 
			updateSize(data);
		}
		bool testClass(CData *data);		
		
		double getBenefit();
		void dump();
		//void updateBenefit();
		
	private:
		inline void updateSize(CData *data) { if (data->size() > m_nSize ) m_nSize = data->size(); }
		int computeCost();
		
	public:
		std::set<MemoryCost> m_MemoryCostSet;	
		std::list<CData *> m_Dataset;
	private:
		unsigned int m_nSize;
		double m_dBenefit;
	};

	class SpmAllocator {
	public:
		int run(const MachineFunction *, const AccessFrequency *, const InterfereGraph *);
		
	private:
		int GetConfig();
		int GetAccess(const AccessFrequency *);
		int GetInterfere(const InterfereGraph *);
		int ComputeDataCost(const MachineFunction *MF);
	
	private:
		// intermediate data structure
		map<int, CData *> t_hData;      // mapping from the nReg to Data
		
	};
	//inline ofstream &AFout() { return g_AFout; }
	//inline ofstream &IGout() { return g_IGout; }
	extern map<enum MemoryKind, CMemory *> g_PwaMemory;
	extern map<enum MemoryKind, CMemory *> g_FcfsMemory;
	extern map<const MachineFunction *, list<CData *> > g_hDataList;
	extern int g_nNumOfMemory;
	extern string MemoryName(enum MemoryKind kind);
	void dumpMemory(std::map<enum MemoryKind, CMemory *> &memoryList, ostream &OS );
	void dumpDatasetList(std::list<CDataClass *> &dclist);
	
}

#endif