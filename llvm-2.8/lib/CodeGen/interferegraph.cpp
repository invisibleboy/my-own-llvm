#include "llvm/CodeGen/interferegraph.h"
#include "llvm/Function.h"
#include "llvm/CodeGen/MachineMemOperand.h"
#include "llvm/Support/Debug.h"
#include "llvm/Target/TargetRegisterInfo.h"
#include "llvm/Target/TargetInstrInfo.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Support/raw_ostream.h"

#undef DEBUG_TYPE
#define DEBUG_TYPE "ig"

namespace llvm
{

char InterfereGraph::ID = 0;
InterfereGraph* InterfereGraph::ms_instance = 0;





InterfereGraph* InterfereGraph::Instance()
{
	if(ms_instance == 0){
		ms_instance = new InterfereGraph();
	}
	return ms_instance;
}

void InterfereGraph::Release()
{
	if(ms_instance){
		delete ms_instance;
	}
	ms_instance = 0;
}

// Two live ranges, [a1, b1) and [a2, b2), interfere with each other, when
// a1 < b2 && a2 < b1
bool InterfereGraph::runOnMachineFunction(MachineFunction& mf)
{
	typedef DenseMap<unsigned, LiveInterval*> Reg2IntervalMap;
	MF = &mf;
	Reg2IntervalMap tmpMap1 = li_->r2iMap_;
	Reg2IntervalMap tmpMap2 = li_->r2iMap_;
	for( Reg2IntervalMap::const_iterator I1 = tmpMap1.begin(), E1 = tmpMap1.end(); I1 != E1; ++ I1 )
	{
		LiveInterval * li1 = I1->second;
		if( li1->isStackSlot() )
			continue;
		if( tri_ && TargetRegisterInfo::isPhysicalRegister(li1->reg) )
			continue;
		for( LiveInterval::Ranges::const_iterator RI1 = li1->ranges.begin(), RE1 = li1->ranges.end();
			RI1 != RE1; ++ RI1)	
		{
			for( Reg2IntervalMap::const_iterator I2 = tmpMap2.begin(), E2 = tmpMap2.end(); I2 != E2; ++ I2 )
			{
				LiveInterval * li2 = I2->second;
				if( li1 == li2 )
					continue;
				if( li2->isStackSlot() )
					continue;
				if( tri_ && TargetRegisterInfo::isPhysicalRegister(li2->reg) )				
					continue;
				for( LiveInterval::Ranges::const_iterator RI2 = li2->ranges.begin(), RE2 = li2->ranges.end();
					RI2 != RE2; ++ RI2)	
				{
					DEBUG( RI1->print(dbgs()) );
					DEBUG( RI2->print(dbgs()) );
					DEBUG( dbgs() << "\n");
					if( RI1->start < RI2->end  && RI2->start < RI1->end )
					{
						m_IGraph[li1->reg].insert(li2->reg);
						m_IGraph[li2->reg].insert(li1->reg);
						break;						
					}
					else if( RI1->start >= RI2->end)
					{
						;
					}
					else if( RI2->start >= RI1->end )
					{
						break;
					}
				}
			}
		}
	}
	dump();
	return true;
}

void InterfereGraph::dump()
{
	print(dbgs());
}

void InterfereGraph::print(raw_ostream &OS) const
{
	map<int, set<int> >::const_iterator i2s_p = m_IGraph.begin();
	OS << "Interfere Graph for function ";
	OS << MF->getFunction()->getName() << "\n";
	for(; i2s_p != m_IGraph.end(); i2s_p ++ )
	{
		int nReg = (i2s_p->first);
		OS << "%reg";
		
		OS << nReg;
		OS << ":\t";
		for( set<int>::iterator s_p = i2s_p->second.begin(), e_p = i2s_p->second.end(); 
			s_p != e_p; s_p ++ )
			OS << *s_p << ",";
		OS << "\n";
	}
}
}

