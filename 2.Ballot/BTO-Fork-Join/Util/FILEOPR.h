/********************************************************************
|  *****     File:   FILEOPR.h                               *****  |
|  *****  Created:   on July 23, 2018,01:46 AM               *****  |
********************************************************************/

//#pragma once
#ifndef _FILEOPR_h
#define _FILEOPR_h
 

#include <fstream>
#include <cstring>
#include <random>
#include <vector>
#include <sstream>
#include <iostream>

using namespace std;
class FILEOPR
{
	public: 
	FILEOPR(){};//constructor
	
	float getRBal( );
	int getRId( int numSObj );
	int getRFunC( int nCFun );
		
	
	void getInp(int* nProposal, int* nVoter,
				int *nThreads, int* nAUs, double* lemda);
	
	void writeOpt( int nProposal, int nVoter, int nThreads, 
					int nAUs, double TTime[], float_t mTTime[], 
					float_t vTTime[], int aCount[], int vAUs, 
					list<double>&mIT, list<double>&vIT );
	
	void genAUs(int numAUs, int numVoter, int numProposal,
				int nFunC, vector<string>& ListAUs
				);
	
	void printCList(int AU_ID, list<int>&CList);
	
	void pAUTrns(std::atomic<int> *mAUTrns, int numAUs);
	
	~FILEOPR() { };//destructor
};
#endif

