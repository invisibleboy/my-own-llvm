#include "interferegraph.h"

namespace llvm
{

InterfereGraph* InterfereGraph::ms_instance = 0;

InterfereGraph::InterfereGraph()
{
}

InterfereGraph::~InterfereGraph()
{
}

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

bool InterfereGraph::runOnMachineFunction(MachineFunction& MF)
{
}
}

