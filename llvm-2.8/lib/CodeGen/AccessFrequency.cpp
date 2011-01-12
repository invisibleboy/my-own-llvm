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


using namespace llvm;

char AccessFrequency::ID = 0;
INITIALIZE_PASS(AccessFrequency, "accfreq",
                "Access Frequency Analysis", false, false);

#undef DEBUG_TYPE
#define DEBUG_TYPE "accfreq"
AccessFrequency::~AccessFrequency()
{
    //dtor
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
    MF = &mf;
    MRI = &mf.getRegInfo();
    TRI = MF->getTarget().getRegisterInfo();

    for (MachineFunction::const_iterator FI = MF->begin(), FE = MF->end();
       FI != FE; ++FI)
    {

        for (MachineBasicBlock::const_iterator BBI = FI->begin(), BBE = FI->end();
            BBI != BBE; ++BBI)
        {
            //MachineInstr *MI = BBI;
            for (unsigned i = 1, e = BBI->getNumOperands(); i != e; i += 2)
            {
                const MachineOperand &MO = BBI->getOperand(i);
// TODO (qali#1#): To hack other kinds of MachineOperands
                if( !MO.isReg() || MO.getReg() == 0 )
                    continue;
                unsigned MOReg = MO.getReg();
                if( MO.isUse() )
                {
                    m_ReadMap[MOReg] ++;
                }
                else if( MO.isDef())
                {
                    m_WriteMap[MOReg] ++;
                }
                else
                    assert("Unrecoganized operation in AccessFrequency::runOnMachineFunction!\n");

            }
        }
    }

    DEBUG(dump());
    return true;
}

void AccessFrequency::dump()
{
    print(dbgs());
}

void AccessFrequency::print(raw_ostream &OS) const
{
    OS << "Read Access Frequency for " << MF->getFunction()->getName() << ":\n";
    for( DenseMap<int, unsigned>::const_iterator DMI = m_ReadMap.begin(), DME = m_ReadMap.end();
    DMI != DME; ++DMI)
    {
        OS << "%reg" << DMI->first << ",\t" << DMI->second << "\n";
    }

    OS << "\nWrite Access Frequency for " << MF->getFunction()->getName() << ":\n";
    for( DenseMap<int, unsigned>::const_iterator DMI = m_WriteMap.begin(), DME = m_ReadMap.end();
    DMI != DME; ++DMI)
    {
        OS << "%reg" << DMI->first << ",\t" << DMI->second << "\n";
    }
    return;

}
