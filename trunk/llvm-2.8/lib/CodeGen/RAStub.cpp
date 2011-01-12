#include "llvm/CodeGen/RegAllocRegistry.h"
#include "llvm/CodeGen/RAStub.h"

static RegisterRegAlloc raStub("stub", "stub register allocator", llvm::createStubRegisterAllocator);



bool RAStub::runOnMachineFunction(MachineFunction &mf)
{
	return true;
}

FunctionPass *createStubRegisterAllocator()
{
    return new RAStub();
}


