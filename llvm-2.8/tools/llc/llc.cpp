//===-- llc.cpp - Implement the LLVM Native Code Generator ----------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This is the llc code generator driver. It provides a convenient
// command-line interface for generating native assembly-language code
// or C code, given LLVM bitcode.
//
//===----------------------------------------------------------------------===//

#include "llvm/LLVMContext.h"
#include "llvm/Module.h"
#include "llvm/PassManager.h"
#include "llvm/Pass.h"
#include "llvm/ADT/Triple.h"
#include "llvm/Support/IRReader.h"
#include "llvm/CodeGen/LinkAllAsmWriterComponents.h"
#include "llvm/CodeGen/LinkAllCodegenComponents.h"
#include "llvm/Config/config.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/FormattedStream.h"
#include "llvm/Support/ManagedStatic.h"
#include "llvm/Support/PluginLoader.h"
#include "llvm/Support/PrettyStackTrace.h"
#include "llvm/System/Host.h"
#include "llvm/System/Signals.h"
#include "llvm/Target/SubtargetFeature.h"
#include "llvm/Target/TargetData.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Target/TargetRegistry.h"
#include "llvm/Target/TargetSelect.h"

#include <fstream>
using namespace std;

#ifdef SPM_ALLOCATION
#include "llvm/CodeGen/SPM.h"
using namespace SPM;
#endif

#include "llvm/Function.h"
#include <memory>
using namespace llvm;


// General options for llc.  Other pass-specific options are specified
// within the corresponding llc passes, and target-specific options
// and back-end code generation options are specified with the target machine.
//
///////////////qali for stack size and PCG////////////
extern std::map<std::string, int > g_hStackSize;
extern std::map<std::string, std::set<std::string> > g_hFuncCall;

void printPCG(std::ostream &os);
void printLocks(std::ostream &os);

static cl::opt<std::string>
InputFilename(cl::Positional, cl::desc("<input bitcode>"), cl::init("-"));

static cl::opt<std::string>
OutputFilename("o", cl::desc("Output filename"), cl::value_desc("filename"));

// Determine optimization level.
static cl::opt<char>
OptLevel("O",
         cl::desc("Optimization level. [-O0, -O1, -O2, or -O3] "
                  "(default = '-O2')"),
         cl::Prefix,
         cl::ZeroOrMore,
         cl::init(' '));

static cl::opt<std::string>
TargetTriple("mtriple", cl::desc("Override target triple for module"));

static cl::opt<std::string>
MArch("march", cl::desc("Architecture to generate code for (see --version)"));

static cl::opt<std::string>
MCPU("mcpu",
  cl::desc("Target a specific cpu type (-mcpu=help for details)"),
  cl::value_desc("cpu-name"),
  cl::init(""));

static cl::list<std::string>
MAttrs("mattr",
  cl::CommaSeparated,
  cl::desc("Target specific attributes (-mattr=help for details)"),
  cl::value_desc("a1,+a2,-a3,..."));

static cl::opt<bool>
RelaxAll("mc-relax-all",
  cl::desc("When used with filetype=obj, "
           "relax all fixups in the emitted object file"));

cl::opt<TargetMachine::CodeGenFileType>
FileType("filetype", cl::init(TargetMachine::CGFT_AssemblyFile),
  cl::desc("Choose a file type (not all types are supported by all targets):"),
  cl::values(
       clEnumValN(TargetMachine::CGFT_AssemblyFile, "asm",
                  "Emit an assembly ('.s') file"),
       clEnumValN(TargetMachine::CGFT_ObjectFile, "obj",
                  "Emit a native object ('.o') file [experimental]"),
       clEnumValN(TargetMachine::CGFT_Null, "null",
                  "Emit nothing, for performance testing"),
       clEnumValEnd));

cl::opt<bool> NoVerify("disable-verify", cl::Hidden,
                       cl::desc("Do not verify input module"));


static cl::opt<bool>
DisableRedZone("disable-red-zone",
  cl::desc("Do not emit code that uses the red zone."),
  cl::init(false));

static cl::opt<bool>
NoImplicitFloats("no-implicit-float",
  cl::desc("Don't generate implicit floating point instructions (x86-only)"),
  cl::init(false));

// GetFileNameRoot - Helper function to get the basename of a filename.
static inline std::string
GetFileNameRoot(const std::string &InputFilename) {
  std::string IFN = InputFilename;
  std::string outputFilename;
  int Len = IFN.length();
  if ((Len > 2) &&
      IFN[Len-3] == '.' &&
      ((IFN[Len-2] == 'b' && IFN[Len-1] == 'c') ||
       (IFN[Len-2] == 'l' && IFN[Len-1] == 'l'))) {
    outputFilename = std::string(IFN.begin(), IFN.end()-3); // s/.bc/.s/
  } else {
    outputFilename = IFN;
  }
  return outputFilename;
}

static tool_output_file *GetOutputStream(const char *TargetName,
                                         Triple::OSType OS,
                                         const char *ProgName) {
  // If we don't yet have an output filename, make one.
  if (OutputFilename.empty()) {
    if (InputFilename == "-")
      OutputFilename = "-";
    else {
      OutputFilename = GetFileNameRoot(InputFilename);

      switch (FileType) {
      default: assert(0 && "Unknown file type");
      case TargetMachine::CGFT_AssemblyFile:
        if (TargetName[0] == 'c') {
          if (TargetName[1] == 0)
            OutputFilename += ".cbe.c";
          else if (TargetName[1] == 'p' && TargetName[2] == 'p')
            OutputFilename += ".cpp";
          else
            OutputFilename += ".s";
        } else
          OutputFilename += ".s";
        break;
      case TargetMachine::CGFT_ObjectFile:
        if (OS == Triple::Win32)
          OutputFilename += ".obj";
        else
          OutputFilename += ".o";
        break;
      case TargetMachine::CGFT_Null:
        OutputFilename += ".null";
        break;
      }
    }
  }

  // Decide if we need "binary" output.
  bool Binary = false;
  switch (FileType) {
  default: assert(0 && "Unknown file type");
  case TargetMachine::CGFT_AssemblyFile:
    break;
  case TargetMachine::CGFT_ObjectFile:
  case TargetMachine::CGFT_Null:
    Binary = true;
    break;
  }

  // Open the file.
  std::string error;
  unsigned OpenFlags = 0;
  if (Binary) OpenFlags |= raw_fd_ostream::F_Binary;
  tool_output_file *FDOut = new tool_output_file(OutputFilename.c_str(), error,
                                                 OpenFlags);
  if (!error.empty()) {
    errs() << error << '\n';
    delete FDOut;
    return 0;
  }

  return FDOut;
}

// main - Entry point for the llc compiler.
//
int main(int argc, char **argv) {
  sys::PrintStackTraceOnErrorSignal();
  PrettyStackTraceProgram X(argc, argv);

  // Enable debug stream buffering.
  EnableDebugBuffering = true;

  LLVMContext &Context = getGlobalContext();
  llvm_shutdown_obj Y;  // Call llvm_shutdown() on exit.

  // Initialize targets first, so that --version shows registered targets.
  InitializeAllTargets();
  InitializeAllAsmPrinters();
  InitializeAllAsmParsers();

  cl::ParseCommandLineOptions(argc, argv, "llvm system compiler\n");

  // Load the module to be compiled...
  SMDiagnostic Err;
  std::auto_ptr<Module> M;

  M.reset(ParseIRFile(InputFilename, Err, Context));
  if (M.get() == 0) {
    Err.Print(argv[0], errs());
    return 1;
  }
  Module &mod = *M.get();

  // If we are supposed to override the target triple, do so now.
  if (!TargetTriple.empty())
    mod.setTargetTriple(Triple::normalize(TargetTriple));

  Triple TheTriple(mod.getTargetTriple());
  if (TheTriple.getTriple().empty())
    TheTriple.setTriple(sys::getHostTriple());

  // Allocate target machine.  First, check whether the user has explicitly
  // specified an architecture to compile for. If so we have to look it up by
  // name, because it might be a backend that has no mapping to a target triple.
  const Target *TheTarget = 0;
  if (!MArch.empty()) {
    for (TargetRegistry::iterator it = TargetRegistry::begin(),
           ie = TargetRegistry::end(); it != ie; ++it) {
      if (MArch == it->getName()) {
        TheTarget = &*it;
        break;
      }
    }

    if (!TheTarget) {
      errs() << argv[0] << ": error: invalid target '" << MArch << "'.\n";
      return 1;
    }

    // Adjust the triple to match (if known), otherwise stick with the
    // module/host triple.
    Triple::ArchType Type = Triple::getArchTypeForLLVMName(MArch);
    if (Type != Triple::UnknownArch)
      TheTriple.setArch(Type);
  } else {
    std::string Err;
    TheTarget = TargetRegistry::lookupTarget(TheTriple.getTriple(), Err);
    if (TheTarget == 0) {
      errs() << argv[0] << ": error auto-selecting target for module '"
             << Err << "'.  Please use the -march option to explicitly "
             << "pick a target.\n";
      return 1;
    }
  }

  // Package up features to be passed to target/subtarget
  std::string FeaturesStr;
  if (MCPU.size() || MAttrs.size()) {
    SubtargetFeatures Features;
    Features.setCPU(MCPU);
    for (unsigned i = 0; i != MAttrs.size(); ++i)
      Features.AddFeature(MAttrs[i]);
    FeaturesStr = Features.getString();
  }

  std::auto_ptr<TargetMachine>
    target(TheTarget->createTargetMachine(TheTriple.getTriple(), FeaturesStr));
  assert(target.get() && "Could not allocate target machine!");
  TargetMachine &Target = *target.get();

  // Figure out where we are going to send the output...
  OwningPtr<tool_output_file> Out
    (GetOutputStream(TheTarget->getName(), TheTriple.getOS(), argv[0]));
  if (!Out) return 1;

  CodeGenOpt::Level OLvl = CodeGenOpt::Default;
  switch (OptLevel) {
  default:
    errs() << argv[0] << ": invalid optimization level.\n";
    return 1;
  case ' ': break;
  case '0': OLvl = CodeGenOpt::None; break;
  case '1': OLvl = CodeGenOpt::Less; break;
  case '2': OLvl = CodeGenOpt::Default; break;
  case '3': OLvl = CodeGenOpt::Aggressive; break;
  }

  // Build up all of the passes that we want to do to the module.
  PassManager PM;

  // Add the target data from the target machine, if it exists, or the module.
  if (const TargetData *TD = Target.getTargetData())
    PM.add(new TargetData(*TD));
  else
    PM.add(new TargetData(&mod));

  // Override default to generate verbose assembly.
  Target.setAsmVerbosityDefault(true);

  if (RelaxAll) {
    if (FileType != TargetMachine::CGFT_ObjectFile)
      errs() << argv[0]
             << ": warning: ignoring -mc-relax-all because filetype != obj";
    else
      Target.setMCRelaxAll(true);
  }

  {
    formatted_raw_ostream FOS(Out->os());

    // Ask the target to add backend passes as necessary.
    if (Target.addPassesToEmitFile(PM, FOS, FileType, OLvl, NoVerify)) {
      errs() << argv[0] << ": target does not support generation of this"
             << " file type!\n";
      return 1;
    }

    PM.run(mod);
  }


  // by qali: cache lock  
  std::ofstream fout;
  std::string clFile = mod.getModuleIdentifier() + ".lock";
  fout.open(clFile.c_str());
  printLocks(fout);
  fout.close();
  
  // Dump the function call info
  std::string fcFile = mod.getModuleIdentifier() + ".dot";
  fout.open(fcFile.c_str());
  printPCG(fout);
  fout.close();

	// by qali: spm allocation
#ifdef SPM_ALLOCATION
  // Do spm allocation
  SpmAllocator spmAllocator;
  spmAllocator.run(&mod);
  
  // Dump the spm allocation info
  std::string fileName = mod.getModuleIdentifier() + ".spm";

  fout.setf(std::ios_base::scientific);
  fout.open(fileName.c_str(), std::ios_base::out);

  fout << "number of functions: " << g_nNumOfFuncs << "\t\tnumber of variables: " << g_nNumOfVars << "\t\ttotal var size: " << g_nTotalSize << "\n";
  fout << "##############################################################################################\n";
  fout << "-------------------------------------First Come First Service---------------------------------\n";
  fout << "##############################################################################################\n";
  double dCost = dumpMpcMemory(g_FcfsMemory, fout);
  //double dCost = dumpMpcMemory(g_FcfsMemory, fout);
  std::ofstream CostOut;
  CostOut.setf(std::ios_base::scientific);
  CostOut.open("spm.fcfs", std::ios_base::app);
  CostOut << mod.getModuleIdentifier() << "\t" << dCost << "\n";
  CostOut.close();
  fout << "##############################################################################################\n";
  fout << "-------------------------------------Partition While Allocate---------------------------------\n";
  fout << "##############################################################################################\n";
  dCost = dumpMemory(g_PwaMemory, fout);
  CostOut.open("spm.pwa", std::ios_base::app);
  CostOut << mod.getModuleIdentifier() << "\t" << dCost << "\n";
  CostOut.close();
  fout.close();
#endif

  // Declare success.
  Out->keep();

  return 0;
}

void printPCG(std::ostream &os)
{
    os << "digraph G {\n";
    // node
    std::map<std::string, int>::iterator s2i_p = g_hStackSize.begin(), s2i_e = g_hStackSize.end();
    for(; s2i_p != s2i_e; ++ s2i_p)
        if( s2i_p->second != 0 )
            os << s2i_p->first << "[label=\"" << s2i_p->first << ": " << s2i_p->second << "\"]\n";

    // edge
    std::map<std::string, std::set<std::string> >::iterator f2s_p = g_hFuncCall.begin(), f2s_e = g_hFuncCall.end();
    for(; f2s_p != f2s_e; ++f2s_p)
    {
        if( f2s_p->second.empty() )
            continue;
        if(g_hStackSize[f2s_p->first] == 0 )
            continue;

        std::set<std::string>::iterator f_p = f2s_p->second.begin(), f_e = f2s_p->second.end();
        for(; f_p != f_e; ++ f_p)
            if( g_hStackSize[*f_p] != 0 )
                os << f2s_p->first << " -> " << *f_p << "\n";
        os << "\n";
    }
    os << "}\n";
}

extern std::map<const Function *, std::map<int, double> > g_hF2Locks;
void printLocks(std::ostream &os)
{
	std::map<const Function *, std::map<int, double> >::iterator f2i_p = g_hF2Locks.begin(), f2i_e = g_hF2Locks.end();
	for(; f2i_p != f2i_e; ++ f2i_p)
	{
		const Function *func = f2i_p->first;
		StringRef szRef = func->getName();
		string szName = szRef.str();
		os << "##" << szName << endl;
		std::map<int, double>::const_iterator i2d_p = f2i_p->second.begin(), i2d_e = f2i_p->second.end();
		for(; i2d_p != i2d_e; ++ i2d_p)
		{
			if( i2d_p->second > 9.999999)
				os << i2d_p->first << "\t" << i2d_p->second << endl;
		}
		os << endl;
	}
}
