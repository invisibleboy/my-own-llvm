#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/CodeGen/Passes.h"
#include "llvm/Support/Debug.h"
#include "llvm/Target/TargetRegisterInfo.h"
#include "llvm/Target/TargetInstrInfo.h"
#include "llvm/Target/TargetMachine.h"
#include "PHIElimination.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/CodeGen/AccessFrequency.h"
#include "llvm/Function.h"
#include "llvm/CodeGen/MachineMemOperand.h"
#include "llvm/CodeGen/MachineModuleInfo.h"
#include "llvm/Module.h"
#include "llvm/Analysis/StaticProfilePass.h"
#include <string>

using namespace std;
using namespace llvm;

char AccessFrequency::ID = 0;
INITIALIZE_PASS(AccessFrequency, "accfreq",
                "Access Frequency Analysis", false, false);

#undef DEBUG_TYPE
#define DEBUG_TYPE "accfreq"


extern const std::map<const Function *, std::map<const BasicBlock *, double> > *g_hF2B2Acc;
AccessFrequency *AccessFrequency::ms_instance = 0;
AccessFrequency * AccessFrequency::Instance()
{
	if(ms_instance == 0){
		ms_instance = new AccessFrequency();
	}
	return ms_instance;
}               

void AccessFrequency::Release()
{
	if(ms_instance){
		delete ms_instance;
	}
	ms_instance = 0;
	//if(SPM::AFout().is_open())  
		//SPM::AFout().close();
}

void AccessFrequency::getAnalysisUsage(AnalysisUsage &AU) const
{
    AU.setPreservesAll();
    AU.addPreservedID(PHIElimination::ID);
    //AU.addRequiredID(TwoAddressInstructionPassID);
    MachineFunctionPass::getAnalysisUsage(AU);
}


bool AccessFrequency::runOnMachineFunction(MachineFunction &mf)
{
	string szInfo;
	string szSrcFile = mf.getMMI().getModule()->getModuleIdentifier();
	szSrcFile = szSrcFile + "." + "af";
	raw_fd_ostream afout(szSrcFile.c_str(), szInfo, raw_fd_ostream::F_Append );
    MF = &mf;
    MRI = &mf.getRegInfo();
    TRI = MF->getTarget().getRegisterInfo();
	
	const llvm::Function *fn = mf.getFunction(); 
    for (MachineFunction::const_iterator FI = MF->begin(), FE = MF->end();
       FI != FE; ++FI)
    {
		double dFactor = 0.0;
		const BasicBlock *bb = FI->getBasicBlock();
		if( bb != NULL )
		{
			//const std::map<const Function *, std::map<const BasicBlock *, double> > &hF2B2Acc =(SP->BlockInformation);
			std::map<const Function *, std::map<const BasicBlock *, double> >::const_iterator f2b2acc_p, E = g_hF2B2Acc->end();
			if( (f2b2acc_p = g_hF2B2Acc->find(fn) ) != E )
			{
				std::map<const BasicBlock *, double>::const_iterator b2acc_p, EE = f2b2acc_p->second.end();
				if( (b2acc_p = f2b2acc_p->second.find(bb) ) != EE )
					dFactor = b2acc_p->second;
			}
		}
		if( dFactor == 0.0 )
			dFactor = 1.0;
		
        for (MachineBasicBlock::const_iterator BBI = FI->begin(), BBE = FI->end();
            BBI != BBE; ++BBI)
        {
			DEBUG(BBI->print(dbgs(), NULL ));
            //MachineInstr *MI = BBI;
            for (unsigned i = 0, e = BBI->getNumOperands(); i != e; ++ i)
            {
                const MachineOperand &MO = BBI->getOperand(i);
// TODO (qali#1#): To hack other kinds of MachineOperands
				switch (MO.getType() )
				{
				case MachineOperand::MO_Register:
					if( MO.getReg() != 0
						&& TargetRegisterInfo::isVirtualRegister(MO.getReg()) )
					{
						unsigned MOReg = MO.getReg();
						if( MO.isUse() )
						{							
							m_RegReadMap[MOReg] = m_RegReadMap[MOReg] + dFactor;
						}
						else if( MO.isDef())
						{
							m_RegWriteMap[MOReg] = m_RegWriteMap[MOReg] + dFactor;
						}
						else
							assert("Unrecoganized operation in AccessFrequency::runOnMachineFunction!\n");
					}
					break;
				default:
					break;
				}
            }

			// Analyze the memoperations
			if(!BBI->memoperands_empty() )
			{
				for( MachineInstr::mmo_iterator i = BBI->memoperands_begin(), e = BBI->memoperands_end();
					i != e; ++ i) {
						if( (*i)->isLoad() )
						{
							const char *tmp = (**i).getValue()->getName().data();

							m_StackReadMap[tmp] ++;
						}
						else if( (*i)->isStore())
						{
							const char *tmp = (**i).getValue()->getName().data();
							m_StackWriteMap[tmp] ++;
						}
						else
						{
							assert(false);
							 dbgs() << __FILE__ << __LINE__;
						}
					}
			}
        }
    }	

    print(afout);
	//reset();
    return true;
}

void AccessFrequency::reset()
{
	m_RegReadMap.clear();
	m_RegWriteMap.clear();
	m_StackReadMap.clear();
	m_StackWriteMap.clear();
}

void AccessFrequency::dump()
{
    //print(SPM::AFout());
}

void AccessFrequency::print(raw_ostream &OS) const
{
    OS << "Read Access Frequency for " << MF->getFunction()->getName() << ":\n";
	for( StringMap<double>::const_iterator DMI = m_StackReadMap.begin(), DME = m_StackReadMap.end();
    DMI != DME; ++DMI)
    {
        OS << MF->getFunction()->getName();
		OS << "__";
		OS << DMI->getKey();
		OS << ",\t" << DMI->getValue() << "\n";
    }
    for( DenseMap<int,double>::const_iterator DMI = m_RegReadMap.begin(), DME = m_RegReadMap.end();
    DMI != DME; ++DMI)
    {
        OS << MF->getFunction()->getName() << "%reg" << DMI->first << ",\t" << DMI->second << "\n";
    }

    OS << "\nWrite Access Frequency for " << MF->getFunction()->getName() << ":\n";
	for( StringMap<double>::const_iterator DMI = m_StackWriteMap.begin(), DME = m_StackWriteMap.end();
    DMI != DME; ++DMI)
    {
        OS << MF->getFunction()->getName();
		OS << "__";
		OS << DMI->getKey();
		OS << ",\t" << DMI->getValue() << "\n";
    }
    for( DenseMap<int, double>::const_iterator DMI = m_RegWriteMap.begin(), DME = m_RegWriteMap.end();
    DMI != DME; ++DMI)
    {
        OS << MF->getFunction()->getName() << "%reg" << DMI->first << ",\t" << DMI->second << "\n";
    }
    return;

}
