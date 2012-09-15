#include <fstream>
#include <string>
#include <sstream>
#include <iostream>
using namespace std;

void  GetUserFunc()
{
	std::ifstream inf;
	double topN = 25.0;
	inf.open("main.bc.lock");
	std::string szLine;
	std::string szFunc;
	std::stringstream ss;
	char cLine[256];	
	
	while(inf.getline(cLine, 255) )
	{
		szLine = cLine;
		if( szLine.size() < 4 )
			continue;
		if( szLine.find("##") == 0 )
		{
			szFunc = szLine.substr(2);
		}
		else 
		{	
			int nBlockID;
			double dFreq;
			
			ss.clear();
			ss << szLine;
			ss >> nBlockID;
			ss >> dFreq;
			
			if( dFreq < topN )
				continue;
//			g_UserfuncSet[szFunc].insert(nBlockID);
			cerr << "--" << szFunc << ": " << nBlockID << endl;
		}
	}
	inf.close();
}

int main()
{
	GetUserFunc();
	return 0;
}
