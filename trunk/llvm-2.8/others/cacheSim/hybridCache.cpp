/*BEGIN_LEGAL
Intel Open Source License

Copyright (c) 2002-2011 Intel Corporation. All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.  Redistributions
in binary form must reproduce the above copyright notice, this list of
conditions and the following disclaimer in the documentation and/or
other materials provided with the distribution.  Neither the name of
the Intel Corporation nor the names of its contributors may be used to
endorse or promote products derived from this software without
specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE INTEL OR
ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
END_LEGAL */
//
// @ORIGINAL_AUTHOR: Artur Klauser
// @EXTENDED: Rodric Rabbah (rodric@gmail.com)
//

/*! @file
 *  This file contains an ISA-portable cache simulator
 *  data cache hierarchies
 */


#include "pin.H"

#include <iostream>
#include <fstream>
#include <set>

#include "cacheL2.H"
#include "pin_profile.H"


/* ===================================================================== */
/* Commandline Switches */
/* ===================================================================== */

KNOB<string> KnobOutputFile(KNOB_MODE_WRITEONCE,    "pintool",
    "o", "hybridCache.out", "specify dcache file name");
KNOB<BOOL>   KnobTrackLoads(KNOB_MODE_WRITEONCE,    "pintool",
    "tl", "0", "track individual loads -- increases profiling time");
KNOB<BOOL>   KnobTrackStores(KNOB_MODE_WRITEONCE,   "pintool",
   "ts", "0", "track individual stores -- increases profiling time");
KNOB<UINT32> KnobThresholdHit(KNOB_MODE_WRITEONCE , "pintool",
   "rh", "100", "only report memops with hit count above threshold");
KNOB<UINT32> KnobThresholdMiss(KNOB_MODE_WRITEONCE, "pintool",
   "rm","100", "only report memops with miss count above threshold");
KNOB<UINT32> KnobCacheSize(KNOB_MODE_WRITEONCE, "pintool",
    "c","512", "cache size in kilobytes");
KNOB<UINT32> KnobLineSize(KNOB_MODE_WRITEONCE, "pintool",
    "b","64", "cache block size in bytes");
KNOB<UINT32> KnobAssociativity(KNOB_MODE_WRITEONCE, "pintool",
    "a","8", "cache associativity (1 for direct mapped)");

/* ===================================================================== */
/* Print Help Message                                                    */
/* ===================================================================== */

INT32 Usage()
{
    cerr <<
        "This tool represents a cache simulator.\n"
        "\n";

    cerr << KNOB_BASE::StringKnobSummary() << endl;
    return -1;
}

/* ===================================================================== */
/* Global Variables */
/* ===================================================================== */

namespace L2        // 3 + 10 + 6 = 19, 512K
{
    const UINT32 max_sets = KILO; // cacheSize / (lineSize * associativity);
    const UINT32 max_associativity = 8; // associativity;
    const CACHE_ALLOC::STORE_ALLOCATION allocation = CACHE_ALLOC::STORE_ALLOCATE;

    typedef CACHE<CACHE_SET::ROUND_ROBIN<max_associativity>, max_sets, allocation> CACHE;
}

namespace IL1        // 3 + 6 + 4, 8k
{
    const UINT32 max_sets = KILO; // cacheSize / (lineSize * associativity);
    const UINT32 max_associativity = 8; // associativity;
    const CACHE_ALLOC::STORE_ALLOCATION allocation = CACHE_ALLOC::STORE_NO_ALLOCATE;

    typedef CACHE_SET::LRU_CACHE_SET<max_associativity> CacheSet;

    typedef CACHE1<CacheSet, max_sets, allocation> CACHE;
}

namespace DL1        // 2 + 6 + 5, 8k
{
    const UINT32 max_sets = KILO; // cacheSize / (lineSize * associativity);
    const UINT32 max_associativity = 8; // associativity;
    const CACHE_ALLOC::STORE_ALLOCATION allocation = CACHE_ALLOC::STORE_NO_ALLOCATE;

    typedef CACHE1<CACHE_SET::LRU_CACHE_SET<max_associativity>, max_sets, allocation> CACHE;
}




// wrap configuation constants into their own name space to avoid name clashes

IL1::CACHE* il1 = NULL;
DL1::CACHE* dl1 = NULL;
L2::CACHE* l2 = NULL;


typedef enum
{
    COUNTER_MISS = 0,
    COUNTER_HIT = 1,
    COUNTER_NUM
} COUNTER;

unsigned int g_nHeapBegin = 0;
set<string> g_userFuncs;

typedef  COUNTER_ARRAY<UINT64, COUNTER_NUM> COUNTER_HIT_MISS;


// holds the counters with misses and hits
// conceptually this is an array indexed by instruction address
COMPRESSOR_COUNTER<ADDRINT, UINT32, COUNTER_HIT_MISS> profile;

set<ADDRINT> tmpIns;
/* ===================================================================== */

//VOID LoadMulti(ADDRINT addr, UINT32 size, UINT32 instId, void * pInst, BOOL bStack, BOOL bMain)
VOID LoadMulti(ADDRINT addr, UINT32 size, bool bUser)
{
//    string szIns = (const char *)szFunc;
////    if( bMain)
//    {
//        if( bStack)
//            cout << "----Stack:";
//        else if( addr < g_nHeapBegin )
//            cout << "----Global:";
//        else
//            cout << "----Heap:";
//        cout << instId << ":load multi:\t" << szIns << "----0x" << hex << addr << dec << endl;
//    }

    //assert("Load multiple" && false);
    // first level D-cache
    dl1->Access(addr, size, CACHE_BASE1::ACCESS_TYPE_LOAD, bUser, true);

//    const COUNTER counter = dl1Hit ? COUNTER_HIT : COUNTER_MISS;
//    profile[instId][counter]++;
}

/* ===================================================================== */

//VOID StoreMulti(ADDRINT addr, UINT32 size, UINT32 instId, void * pInst, BOOL bStack, BOOL bMain)
VOID StoreMulti(ADDRINT addr, UINT32 size, bool bUser)
{
   // string szIns = (const char *)szFunc;
////    if( bMain)
//    {
//        if( bStack)
//            cout << "----Stack:";
//        else if( addr < g_nHeapBegin )
//            cout << "----Global:";
//        else
//            cout << "----Heap";
//        cout << instId << ":store multi:\t" << szIns << "----0x" << hex << addr << dec << endl;
//    }

    //assert("Store multiple" && false);
    // first level D-cache
    dl1->Access(addr, size, CACHE_BASE1::ACCESS_TYPE_STORE, bUser, true);

//    const COUNTER counter = dl1Hit ? COUNTER_HIT : COUNTER_MISS;
//    profile[instId][counter]++;
}

/* ===================================================================== */

VOID LoadInstruction(ADDRINT addr, bool bUser)
{
    il1->AccessSingleLine(addr, CACHE_BASE1::ACCESS_TYPE_LOAD, bUser, false);
}
//VOID LoadSingle(ADDRINT addr, UINT32 instId, void * pInst, BOOL bStack, BOOL bMain)
VOID LoadSingle(ADDRINT addr, bool bUser)
{
//    string szIns = (const char *)szFunc;
    //if( bUser)
//    {
//        if( bStack)
//            cout << "----Stack:";
//        else if( addr < g_nHeapBegin )
//            cout << "----Global:";
//        else
//            cout << "----Heap";
//        cout << instId << ":load single:\t" << szIns << "----0x" << hex << addr << dec << endl;
//        cout <<"##Address:\t" << (addr >> 6) << "\n";
//    }
//    ADDRINT tag = addr >> 6;
//    if( g_hShift[tag] > 5000 && tmpIns.find(iAddr) == tmpIns.end() )
//    {
//        cout << tag << ":" << hex << iAddr << dec << ":" << g_hShift[tag] << "\n";
//        tmpIns.insert(iAddr);
//    }

    // @todo we may access several cache lines for
    // first level D-cache
    dl1->AccessSingleLine(addr, CACHE_BASE1::ACCESS_TYPE_LOAD, bUser, true);

}
/* ===================================================================== */

//VOID StoreSingle(ADDRINT addr, UINT32 instId, void * pInst, BOOL bStack, BOOL bMain)
VOID StoreSingle(ADDRINT addr, bool bUser)
{
//    string szIns = (const char *)szFunc;
    //if( bUser)
//    {
//        if( bStack)
//            cout << "----Stack:";
//        else if( addr < g_nHeapBegin )
//            cout << "----Global:";
//        else
//            cout << "----Heap";
//        cout << instId << ":store single:\t" << szIns << "----0x" << hex << addr << dec << endl;
//        cout <<"##Address:\t" << (addr >> 6) << "\n";
//    }

    // @todo we may access several cache lines for
    // first level D-cache
    dl1->AccessSingleLine(addr, CACHE_BASE1::ACCESS_TYPE_STORE, bUser, true);

//    const COUNTER counter = dl1Hit ? COUNTER_HIT : COUNTER_MISS;
//    profile[instId][counter]++;
}

/* ===================================================================== */

VOID LoadMultiFast(ADDRINT addr, UINT32 size)
{
    //dl1->Access(addr, size, CACHE_BASE::ACCESS_TYPE_LOAD, true);
}

/* ===================================================================== */

VOID StoreMultiFast(ADDRINT addr, UINT32 size)
{
    //dl1->Access(addr, size, CACHE_BASE::ACCESS_TYPE_STORE, true);
}

/* ===================================================================== */

VOID LoadSingleFast(ADDRINT addr)
{
    //dl1->AccessSingleLine(addr, CACHE_BASE::ACCESS_TYPE_LOAD, true);
}

/* ===================================================================== */

VOID StoreSingleFast(ADDRINT addr)
{
    //dl1->AccessSingleLine(addr, CACHE_BASE::ACCESS_TYPE_STORE, true);
}


VOID Image(IMG img, VOID *v)
{
    RTN mainRtn = RTN_FindByName(img, "main");
    if( RTN_Valid(mainRtn))
    {
        g_nHeapBegin = IMG_HighAddress(img);
        //cout << "<<<<<<<<<<<<<Begin of Heap address:\t" << g_nHeapBegin << ">>>>>>>>>>>>>>\n";
    }
}

/* ===================================================================== */

VOID Instruction(INS ins, void * v)
{
    //string szIns = "";
//    bool bMain = false;
     const ADDRINT iaddr = INS_Address(ins);
    // const UINT32 instId = profile.Map(iaddr);
     //string szIns = INS_Disassemble(ins);
////     bool bMain = false;
    // RTN rtn = INS_Rtn(ins);
    // string szFunc = "";
////     bool bUser = false;
     //if( RTN_Valid(rtn) )
   //  {
     //    szFunc = RTN_Name(rtn);
////        if( g_userFuncs.find(szFunc) != g_userFuncs.end())
////            bUser = true;
         //if( szFunc == "main")
//         {
           //  cout << instId << ":" << szIns << "###\n";
////             //cout << szFunc << endl;
//////             bMain = true;
//////
//             SEC sec = RTN_Sec(rtn);
//             IMG img = SEC_Img(sec);
//             g_nHeapBegin = IMG_HighAddress(img);
//             cout << "<<<<<<<<<<<<<Begin of Heap address:\t" << g_nHeapBegin << ">>>>>>>>>>>>>>\n";
//         }
//////         cout << IMG_Name(img) << hex << "(" << IMG_LowAddress(img) << "," << IMG_HighAddress(img) <<dec << ")::" << SEC_Name(sec) << "::" << szFunc << endl;
    // }

    INS_InsertPredicatedCall(
                    ins, IPOINT_BEFORE, (AFUNPTR) LoadInstruction,
                    IARG_UINT32, iaddr,
                    IARG_BOOL, true,
                    IARG_END);

    if (INS_IsMemoryRead(ins))
    {
        RTN rtn = INS_Rtn(ins);
        string szFunc = "";
        bool bUser = false;
        if( RTN_Valid(rtn) )
        {
            szFunc = RTN_Name(rtn);
           if( g_userFuncs.find(szFunc) != g_userFuncs.end())
               bUser = true;
        }

        const UINT32 size = INS_MemoryReadSize(ins);
        const BOOL   single = (size <= 4);

        //const BOOL bStack = INS_IsStackRead(ins)? true: false;

        if( KnobTrackLoads )
        {
            if( single )
            {
                INS_InsertPredicatedCall(
                    ins, IPOINT_BEFORE, (AFUNPTR) LoadSingle,
                    IARG_MEMORYREAD_EA,
   //                 IARG_UINT32, iaddr,
                    IARG_BOOL, bUser,
//                    IARG_BOOL, bStack,
                    //IARG_BOOL, bMain,
                    IARG_END);
            }
            else
            {
                INS_InsertPredicatedCall(
                    ins, IPOINT_BEFORE,  (AFUNPTR) LoadMulti,
                    IARG_MEMORYREAD_EA,
                    IARG_MEMORYREAD_SIZE,
      //              IARG_UINT32, iaddr,
                    IARG_BOOL, bUser,
//                    IARG_BOOL, bStack,
//                    IARG_BOOL, bMain,
                    IARG_END);
            }

        }
        else
        {
            if( single )
            {
                INS_InsertPredicatedCall(
                    ins, IPOINT_BEFORE,  (AFUNPTR) LoadSingleFast,
                    IARG_MEMORYREAD_EA,
                    IARG_END);

            }
            else
            {
                INS_InsertPredicatedCall(
                    ins, IPOINT_BEFORE,  (AFUNPTR) LoadMultiFast,
                    IARG_MEMORYREAD_EA,
                    IARG_MEMORYREAD_SIZE,
                    IARG_END);
            }
        }
    }

    if ( INS_IsMemoryWrite(ins) )
    {
        RTN rtn = INS_Rtn(ins);
        string szFunc = "";
        bool bUser = false;
        if( RTN_Valid(rtn) )
        {
            szFunc = RTN_Name(rtn);
           if( g_userFuncs.find(szFunc) != g_userFuncs.end())
               bUser = true;
        }

        const UINT32 size = INS_MemoryWriteSize(ins);

        const BOOL   single = (size <= 4);

        //const BOOL bStack = INS_IsStackWrite(ins)? true: false;

        if( KnobTrackStores )
        {
            if( single )
            {
                INS_InsertPredicatedCall(
                    ins, IPOINT_BEFORE,  (AFUNPTR) StoreSingle,
                    IARG_MEMORYWRITE_EA,
         //           IARG_UINT32, iaddr,
                    IARG_BOOL, bUser,
//                    IARG_BOOL, bStack,
//                    IARG_BOOL, bMain,
                    IARG_END);
            }
            else
            {
                INS_InsertPredicatedCall(
                    ins, IPOINT_BEFORE,  (AFUNPTR) StoreMulti,
                    IARG_MEMORYWRITE_EA,
                    IARG_MEMORYWRITE_SIZE,
        //            IARG_UINT32, iaddr,
                    IARG_BOOL, bUser,
//                    IARG_BOOL, bStack,
//                    IARG_BOOL, bMain,
                    IARG_END);
            }

        }
        else
        {
            if( single )
            {
                INS_InsertPredicatedCall(
                    ins, IPOINT_BEFORE,  (AFUNPTR) StoreSingleFast,
                    IARG_MEMORYWRITE_EA,
                    IARG_END);

            }
            else
            {
                INS_InsertPredicatedCall(
                    ins, IPOINT_BEFORE,  (AFUNPTR) StoreMultiFast,
                    IARG_MEMORYWRITE_EA,
                    IARG_MEMORYWRITE_SIZE,
                    IARG_END);
            }
        }

    }
}

/* ===================================================================== */

VOID Fini(int code, VOID * v)
{

    std::ofstream out(KnobOutputFile.Value().c_str());

    // print D-cache profile
    // @todo what does this print

    out << "PIN:MEMLATENCIES 1.0. 0x0\n";

    out << "#\n"
        "# DCACHE L1 stats\n"
        "#\n";
    out << "Cache Size:\t" << dl1->CacheSize() << "\n";
    out << "Line Size:\t" << dl1->LineSize() << "\n";
    out << "Associativity:\t" << dl1->Associativity() << "\n";
    out << "#\n";
    out << dl1->StatsLong("# ", CACHE_BASE1::CACHE_TYPE_DCACHE);

    out << "\n\n";


    out <<
        "#\n"
        "# Unified L2 stats\n"
        "#\n";
    out << "#\n";
    out << "# Cache Size:\t" << KnobCacheSize.Value() * KILO << "\n";
    out << "# Line Size:\t" << KnobLineSize.Value() << "\n";
    out << "# Associativity:\t" << KnobAssociativity.Value() << "\n";
    out << "#\n";
    out << l2->Dump("# ", CACHE_BASE1::CACHE_TYPE_DCACHE);

//    if( KnobTrackLoads || KnobTrackStores ) {
//        out <<
//            "#\n"
//            "# LOAD stats\n"
//            "#\n";
//
//        out << profile.StringLong();
//    }
    out.close();
}

/* =============================================-gstabs======================== */
/* Main                                                                  */
/* ===================================================================== */

int main(int argc, char *argv[])
{
    PIN_InitSymbols();

    if( PIN_Init(argc,argv) )
    {
        return Usage();
    }

    il1 = new IL1::CACHE("L1 Inst Cache", 16 * KILO, 32, 8);
    dl1 = new DL1::CACHE("L1 Data Cache", 16 * KILO, 64, 8);
    l2 = new L2::CACHE("L2 cache", 512 * KILO, 64, 8);

    il1->SetNextLevel(l2);
    dl1->SetNextLevel(l2);

    profile.SetKeyName("iaddr          ");
    profile.SetCounterName("dcache:miss        dcache:hit");

    COUNTER_HIT_MISS threshold;

    threshold[COUNTER_HIT] = KnobThresholdHit.Value();
    threshold[COUNTER_MISS] = KnobThresholdMiss.Value();

    profile.SetThreshold( threshold );

    IMG_AddInstrumentFunction(Image, 0);
    INS_AddInstrumentFunction(Instruction, 0);
    PIN_AddFiniFunction(Fini, 0);

    ifstream infile;
    infile.open("userfunc", ifstream::in);
    string szFunc;
    while(infile.good())
    {
        infile >> szFunc;
        if( !szFunc.empty())
            g_userFuncs.insert(szFunc);
    }
    infile.close();
    // Never returns

    PIN_StartProgram();

    return 0;
}

/* ===================================================================== */
/* eof */
/* ===================================================================== */
