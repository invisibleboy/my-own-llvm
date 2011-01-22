#ifndef LLVM_SPM_H
#define LLVM_SPM_H

#include <map>
#include <list>
#include <vector>
#include <string>
#include <set>
#include <fstream>
#include "stdio.h"
#include "llvm/Support/raw_ostream.h" 
using namespace llvm;
using namespace std;


namespace SPM
{	
	class MachineFunction;
	class CData;
	
	enum MemoryKind
	{
		PCM,
		SRAM,
		DRAM
	};

	struct AllocNode
	{
		unsigned int m_nSize;
		set<CData *> m_Data_set;
		set<MachineFunction *> m_Function_set;
	};
	
	class CMemory
	{
	public:
		enum MemoryKind  m_kind;
		float m_fCostOfRead;
		float m_fCostOfWrite;
		unsigned int m_nSize;
		unsigned int m_nReserved;
		list<AllocNode *> m_Alloc_list;

	public:
		CMemory( enum MemoryKind kind, float costOfRead, float costOfWrite, unsigned int nSize)
		{
			m_kind = kind;
			m_fCostOfRead = costOfRead;
			m_fCostOfWrite = costOfWrite;
			m_nSize = nSize;
			m_nReserved = nSize;
		}
	};

	struct MemoryCost
	{
		enum MemoryKind m_kind;
		float m_fCost;

		bool operator < (const struct MemoryCost &mc) const
		{
			return m_fCost < mc.m_fCost;
		}
	};

	class CData
	{
	public:
		string m_szName;
		double m_dReads;
		double m_dWrites;
		unsigned int m_nSize;

		set<struct MemoryCost> m_MemoryCostSet;

	public:
		CData()
		{
			m_dReads = 0.0;
			m_dWrites = 0.0;
			m_nSize = 0;
		}
		CData(string szName, double dReads, double dWrites, unsigned int nSize)
		{
			m_szName = szName;
			m_dReads = dReads;
			m_dWrites = dWrites;
			m_nSize = nSize;
		}

		void SetName(string szFunc, int nReg)
		{
			char digits[6];
			sprintf(digits, "%d", nReg);
			m_szName = szFunc + "#" + digits;
		}
	};

	//inline ofstream &AFout() { return g_AFout; }
	//inline ofstream &IGout() { return g_IGout; }
	extern list<CMemory *> g_Memory_list;
}

#endif
