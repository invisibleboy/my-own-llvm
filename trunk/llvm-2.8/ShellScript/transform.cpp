/*
This file is used to transform the .sol file from cplex to the file .alloc which is used for volatile STT-RAM allocation
.sol: <variable name="x#dwIndex#blockIndex" index="19650" value="1"/>
.alloc: <dwID, blockID, 1>
An auxiliary file, .traceILP is required to associate dataIndex with dataId, adn blockIndex with blockID
.traceILP: <cycle, dwID, func, size>
==dwIndex is the numberID of dwID in trace
==blockIndex is the blockID from 0
*/


#include <iostream>
#include <map>
#include <string>
#include <sstream>
#include <fstream>
#include <set>
#include <list>

using namespace std;

map<int, int> g_hID2Index;
map<int, int> g_hIndex2ID;

map<int, int> g_hData2Block;
map<int, set<int> > g_hBlock2Data;

int g_nBlocks = 0;
int GetMapinfo(string szFile);
int ReadSolFile(string szFile);
int WriteAllocFile(string szFile);

int ReadAllocFile(string szFile);
int WriteSolFile(string szFile);


int main(int argc, char **argv)
{
	if( argc < 3)
	{
		cerr << "Lack of arguments!" << endl;
		cerr <<"transform {file name} {1: .sol->.alloc; 2: .alloc->.sol}" << endl;
		return -1;
	}

	string szFile = argv[1];
	int nDirection;
	stringstream ss(argv[2]);
	ss >> nDirection;
	
	GetMapinfo(szFile);
	if( nDirection == 1 )
	{
		ReadSolFile(szFile);
		WriteAllocFile(szFile);
	}
	else
	{
		ReadAllocFile(szFile);
		WriteSolFile(szFile);
	}
}

int WriteSolFile(string szFile)
{
    // write .sol file
	
	int index = szFile.find('_');
	index = szFile.find('_', index+1);
    std::string fileName = szFile.substr(0,index) + ".sol";
    ofstream outf;
    outf.open(fileName.c_str());
    if( !outf.good() )
        cerr << "Failed to open " << fileName << endl;
    outf << "<?xml version = \"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>" << endl;
    outf << "<CPLEXSolution version=\"1.2\">" << endl;
    outf << "\t<header" << endl;
    outf << "\t\tproblemName=\"" << szFile << ".lp" << "\"" << endl;
    outf << "\t\tsolutionStatusString=\"integer feasible solution\"" << endl;
    outf << "\t\tsolutionMethodString=\"mip\"/>" << endl;
    
    outf << "\t<variables>" << endl;
    map<int,int>::iterator i2d_p = g_hIndex2ID.begin(), i2d_e = g_hIndex2ID.end();
    for(; i2d_p != i2d_e; ++ i2d_p )
    {
          int nDataIndex = i2d_p->first;
          int nDataID = i2d_p->second;
          int nBlock = g_hData2Block[nDataID];
          outf << "\t\t<variable name=\"x#";
          outf << nDataIndex << '#' << nBlock << "\" value=\"1\"/>" << endl;
    }
    outf << "\t</variables>" << endl;
    outf << "</CPLEXSolution>" << endl;
    outf.close();
    cerr << "#write " << fileName << endl;
    return 0;
}

int ReadAllocFile(string szFile)
{
    std::string fileName = szFile + ".alloc";
    ifstream inf;
    inf.open(fileName.c_str());
    if( !inf.good() )
        cerr << "Failed to open " << fileName << endl;
    std::string szLine;
    int nBlock = -1;
    while(inf.good())
    {
     std::getline(inf,szLine);
     if(szLine.size() < 4 )
         continue;
     if( szLine.find("###") == 0 )
     {
         int nFunc;
         stringstream ss(szLine.substr(3));
         ss >> nFunc;
         if( nFunc == 1 )
             continue;
         else
             break;
     }
     
     // read a block within a line
     int index = szLine.find(":");
     if( index != string::npos)
     {
         ++ nBlock;
         szLine = szLine.substr(index+1);
         
     }
     while( (index = szLine.find(";") ) != string::npos )
     {
         string szData = szLine.substr(0,index);
         int nData;
         stringstream ss(szData);
         ss >> nData;
         g_hBlock2Data[nBlock].insert(nData);
         g_hData2Block[nData] = nBlock;
         szLine = szLine.substr(index+1);
         if( szLine.size() < 4 )
             break;
     }   
 }  
	inf.close();
	 cerr << "#read " << fileName << endl;
    return 0;
}

int WriteAllocFile(string szFile)
{
    // write .alloc file
    std::string fileName = szFile + ".alloc";
    ofstream outf;
    outf.open(fileName.c_str());
    if( !outf.good() )
        cerr << "Failed to open " << fileName << endl;
    outf << "###1" << endl;    
    
    map<int, set<int> >::iterator d2b_p = g_hBlock2Data.begin(), d2b_e = g_hBlock2Data.end();
    for(; d2b_p != d2b_e; ++ d2b_p )
    {
     int nDataID = d2b_p->first;     
     
     set<int> &dataSet = d2b_p->second;
     set<int>::iterator d_p = dataSet.begin(), d_e = dataSet.end();
     outf << *d_p << ":\t";
     for(; d_p != d_e; ++ d_p )
               outf << *d_p << "; ";
     outf << endl;
    }
	outf.close();
	 cerr << "#write " << fileName << endl;
    return 0;
}


int ReadSolFile(string szFile)
{
    // read .sol file
    std::string fileName = szFile + ".sol";
    ifstream inf;
    inf.open(fileName.c_str());
    if( !inf.good() )
        cerr << "Failed to open " << fileName << endl;
    std::string szLine;
    int nCount = 0;
    while(inf.good())
    {
     std::getline(inf,szLine);
     int index = szLine.find("<variable name=\"x#");
     if( index == string::npos )
         continue;
     // parser the variable numbering with dataID and blockID
     string szVariable = szLine.substr(index+16);
     index = szVariable.find('#');
     string szVar = szVariable.substr(0,index);
     
     szVariable = szVariable.substr(index+1);
     index = szVariable.find('#');
     string szDataIndex = szVariable.substr(0,index);
     
     szVariable = szVariable.substr(index+1);
     index = szVariable.find('"');
     string szBlockIndex = szVariable.substr(0,index);
     
     int nDataIndex;
     stringstream ss(szDataIndex);
     ss >> nDataIndex;
     int nDataID;
     nDataID = g_hIndex2ID[nDataIndex]; 
     
     int nBlockIndex; 
     stringstream ss1(szBlockIndex);
     ss1 >> nBlockIndex;     
     
     if( nDataID == 0 || nDataID == 1 )
         continue;
     
     g_hData2Block[nDataID] = nBlockIndex;
     g_hBlock2Data[nBlockIndex].insert(nDataID);
     }
     
	inf.close();
	 cerr << "#read " << fileName << endl;
    return 0;
}


int GetMapinfo(string szFile)
{
    //read .traceILP_1 file
    std::string fileName = szFile + ".dat";
    ifstream inf;
    inf.open(fileName.c_str());
    if( !inf.good() )
        cerr << "Failed to open " << fileName << endl;
    std::string szLine;
    int nCount = 0;
    while(inf.good())
    {
     std::getline(inf,szLine);
     if( szLine.find("nBlocks") != string::npos)
     {
         szLine = szLine.substr(10);
         int index = szLine.find(';');
         szLine = szLine.substr(0,index);
         stringstream ss(szLine);
         ss >> g_nBlocks;
     }
     if( szLine.find('<') == string::npos )
         continue;
     szLine = szLine.substr(1);
     int dataID;    

	 int index = szLine.find(',');
		szLine = szLine.substr(index+1);
		index = szLine.find(',');
		szLine = szLine.substr(0,index);     

     stringstream ss(szLine);
     ss >> dataID;
     g_hID2Index[dataID] = nCount;
     g_hIndex2ID[nCount] = dataID;
     ++ nCount;     
    }
    inf.close();  
	 cerr << "#read " << fileName << endl;
    return 0;
}
