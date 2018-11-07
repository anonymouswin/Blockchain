/********************************************************************
|  *****     File:   FILEOPR.h                               *****  |
|  *****  Created:   July 24, 2018, 01:46 AM                 *****  |
********************************************************************/

#include "FILEOPR.h"

/*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
/*!!! RANDOM NUMBER GENERATER FOR ACCOUNT BALANCE !!!*/
/*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
float FILEOPR::getRBal( ) 
{
	random_device rd;                          //Random seed
	mt19937 gen(rd());                         //Initialize Mersenne Twister pseudo-random number generator
	uniform_int_distribution<> dis( 1, 1000 ); //Uniformly distributed in range (1, 1000)
	int num = dis(gen);
	return num;
}
	
/*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
/*!!! RANDOM NUMBER GENERATER FOR ID !!!*/
/*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
int FILEOPR::getRId( int numSObj) 
{
	random_device rd; 
	mt19937 gen(rd());
	uniform_int_distribution<> dis(1, numSObj); 
	int num = dis(gen);
	return num;
}
	
/*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
/*!!! RANDOM NUMBER GENERATER FOR FUNCTION CALL !!!*/
/*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
int FILEOPR::getRFunC( int nCFun ) 
{
	random_device rd;          
	mt19937 gen(rd());        
	uniform_int_distribution<> dis(1, nCFun);
	int num = dis(gen);
	return num;
}


/*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
!!! getInp() reads #Shared Objects, #Threads, #AUs, & random delay seed "Lemda" from input file !
!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
void FILEOPR::getInp(int* m, int* n, int* K, double* lemda ) 
{
	string ipBuffer[4]; //stores input from file
	ifstream inputFile;
	inputFile.open ( "inp-output/inp-params.txt" );
	while(!inputFile)
	{
		cerr << "Error!! Unable to open inputfile <inp-params.txt>\n\n";
		exit(1); //call system to stop
	}
	int i = 0;
	while( !inputFile.eof( ) )
	{
		inputFile >> ipBuffer[i];
		i++;
	}
	*m     = stoi(ipBuffer[0]);     // m: # of SObj;
	*n     = stoi(ipBuffer[1]);     // n: # of threads should be one for sequential program;
	*K     = stoi(ipBuffer[2]);     // K: Total # of AUs or Transactions;
	*lemda = stof(ipBuffer[3]);     // λ: random delay

	//cout<<"\n[ # Shared Objects = "<< *m <<" ]\n[ # Threads = "<< *n << " ]\n[ # AUs = " << *K << " ]\n[ Lemda = " << *lemda << "]\n\n";
	inputFile.close( );
	return;
}


/*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
!!! writeOpt() stores the Time taken by algorithm in output file "Time.txt"  !
!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
//file_opr.writeOpt(SObj, nThread, numAUs, tTime, mTTime, vTTime, aCount, vAUs, mItrT, vItrT);
void FILEOPR::writeOpt(int m,int n,int K,double total_time[],float_t mTTime[],float_t vTTime[],int aCount[],int validAUs,list<double>&mIT,list<double>&vIT)
{
	float_t minerT = 0;//total time miner thread
	float_t validatorT = 0;//total time validator thred
	
	for(int i = 0; i < n; i++) 
	{
	//	cout<<"THREAD "<< i << " = "<< mTTime[i] <<" microseconds\n";
		minerT = minerT + mTTime[i];
	}

	for(int i = 0; i <= n; i++) 
	{
	//	cout<<"THREAD "<< i << " = "<< vTTime[i] <<" microseconds\n";
		validatorT = validatorT + vTTime[i];
	}

	int total_Abort = 0;	
	for(int i = 0; i < n; i++)
	{
		total_Abort = total_Abort + aCount[i];
	}	
//	cout<<" Total Aborts = "<<total_Abort;

	//Average Time Taken by one Miner Thread = Total Time/# Threads
	cout<<"\n    Avg Miner = "<<minerT/n<<" microseconds\n";
	mIT.push_back(minerT/n);

	//Average Time Taken by one Validator Thread = Total Time/# Threads
	cout<<"Avg Validator = "<<validatorT/n<<" microseconds\n";
	vIT.push_back(validatorT/n);

	//Average Time Taken/AU by Miner = Total Time/# AUs
//	cout<<"\n\nAverage Time Taken per Atomic Unit by Miner     = "<<minerT/K<<" microseconds\n";
	
	//Average Time Taken/AU by Miner = Total Time/# AUs
//	cout<<"Average Time Taken per Atomic Unit by Validator = "<<validatorT/validAUs<<" microseconds\n";
	
	//Total Time Taken by Miner Algorithm = max time taken by a thread
//	cout<<"\nTotal Time taken by Miner     Algorithm  = "<<total_time[0]*1000<<" microseconds\n";
	
	//Total Time Taken by Validator Algorithm = max time taken by a thread
//	cout<<"Total Time taken by Validator Algorithm  = "<<total_time[1]*1000<<" microseconds\n";
	
	//Total Avg Time Taken by Both Algorithm Threads
	cout<<"Total (M + V) = "<<(minerT/n + validatorT/n)<<" microseconds\n";
}
	
	
	
/*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
!!! genAUs() generate and store the Atomic Unites (transactions to be executed by miner/validator) in a list & file !!!
!!! nFunC: parallel fun's (AUs) in smart contract, numAUs: number of AUs to be requested by client to execute       !!!
!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
void FILEOPR::genAUs(int numAUs, int SObj, int nFunC, vector<string>& ListAUs)
{
	//Random_gen r_val;
	int auCount = 1;
	ofstream out_file;
	out_file.open("inp-output/listAUs.txt", std::ios_base::app);
	ListAUs.clear();
	//cout<<"\n---------------------";
	//cout<<"\nAU | Operations\n";
	//cout<<"---------------------\n";
	while(auCount <= numAUs)
	{
		int funName = getRFunC( nFunC );//gives contract func: 1 = "send()" and 2 = "get_bal()"
		if(funName == 1)
		{
				int from = getRId(SObj);
				int to   = getRId(SObj);
				
				while (from == to)
				{
					to = getRId(SObj);
				}
				int ammount = getRBal( );
				
				string trns = to_string(auCount)+" send "+to_string(from)+" "+to_string(to)+" "+to_string(ammount)+"\n";
				//cout<<" "+trns;
				out_file << trns;
				ListAUs.push_back(trns);
				auCount++;
		}
		else if (funName == 2)
		{
				int id      = getRId(SObj);
				string trns = to_string(auCount)+" get_bal "+to_string(id)+"\n";
				
				out_file << trns;
				//cout<<" "+trns;
				ListAUs.push_back(trns);
				auCount++;
		}
	}
	//cout<<"---------------------\n";
	out_file.close ( );
	return;
}

/*---------------------------------------
Print Conflict List of given Atomic Unit.
---------------------------------------*/
void FILEOPR::printCList(int AU_ID, list<int>&CList)
{
	string str;
	
	for(auto it = CList.begin(); it != CList.end(); it++)
	{
		str  = str + to_string(*it)+" ";
	}	
	cout<< " AU_ID- "+to_string(AU_ID)+" Conf_list (time-stamp) [ "+str+"]\n";
	return;
}


/*--------------------------------------------------------------------------------
Print Table used for Mapping AUs with Committed Transaction (that has executed AU)
--------------------------------------------------------------------------------*/
void FILEOPR::pAUTrns(std::atomic<int> *mAUTrns, int numAUs)
{
	cout<<"\n========================\n";
	cout << "  AU_ID |  Timestamp";
	cout<<"\n========================\n";
	
	for (int i = 0; i < numAUs; i++)
	{
		cout  <<  "   " << i+1 <<  "\t|    " << mAUTrns[i] << "\n";
	}
	cout<<"========================\n";	
	return;
}
