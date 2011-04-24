#ifndef LLVM_SPMALLOC_H
#define LLVM_SPMALLOC_H

#include "llvm/CodeGen/SPM.h"

using namespace llvm;
using namespace std;
namespace SPM
{
	// First come first servise heuristics
	class FcfsAlloc
	{
	public:
		FcfsAlloc() { }; 
		
		int Allocator( );   
	private:		
		int Allocate(CData *);
	};	
	
	// Multi-Phase Coloring heuristic
	class MpcAlloc
	{
	public:
		MpcAlloc() { }; 
		
		int Allocator( );       
	private:				
		
		std::list<CData *>::iterator GetSpilled( std::list<CData *> &data_list, std::map<CData *, unsigned int> &hDegree);
		void SetSubgraph( std::list<CData *> &data_list);			

	private:
		std::map<SPM::MemoryKind, std::set<CData *> > m_hSubgraph;
	};	
	
	// Xue's Coloring heuristic
	class XueAlloc
	{
	public:
		XueAlloc() { }; 
		
		int Allocator( );       
	private:				
		
		std::list<CData *>::iterator GetSpilled( std::list<CData *> &data_list, std::map<CData *, unsigned int> &hDegree);
		void SetSubgraph( std::list<CData *> &data_list);		
	};	
}
#endif
