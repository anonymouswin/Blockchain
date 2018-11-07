/********************************************************************
|  *****    Simple Auction MVTO Miner + Fork-Join Validator  *****  |
|  *****     File:   coarse.cpp                              *****  |
|  *****  Created:   Aug 18, 2018, 11:46 AM                  *****  |
*********************************************************************
|  *****  COMPILE:: g++ coarse.cpp -pthread -std=c++14 -ltbb *****  |
********************************************************************/

#include <iostream>
#include <thread>
#include "Util/Timer.cpp"
#include "Contract/SimpleAuction.cpp"
#include "Graph/Coarse/Graph.cpp"
#include "Util/FILEOPR.cpp"

#define maxThreads 128
#define maxBObj 5000
#define maxbEndT 30 //milliseconds
#define funInContract 6
#define pl "=============================\n"

using namespace std;
using namespace std::chrono;

int beneficiary = 0;           //! fixed beneficiary id to 0, it can be any unique address/id.
int    nBidder  = 2;           //! nBidder: number of bidder shared objects.
int    nThread  = 1;           //! nThread: total number of concurrent threads; default is 1.
int    numAUs;                 //! numAUs: total number of Atomic Unites to be executed.
double lemda;                  //! λ: random delay seed.
int    bidEndT;                //! time duration for auction.
double totalT[2];              //! total time taken by miner and validator algorithm respectively.
SimpleAuction *auction = NULL; //! smart contract for miner.
Graph  *cGraph = NULL;         //! conflict grpah generated by miner to be given to validator.
int    *aCount = NULL;         //! aborted transaction count.
float_t *mTTime = NULL;        //! time taken by each miner Thread to execute AUs (Transactions).
float_t *vTTime = NULL;        //! time taken by each validator Thread to execute AUs (Transactions).
float_t *gTtime = NULL;        //! time taken by each miner Thread to add edges and nodes in the conflict graph.
vector<string> listAUs;        //! holds AUs to be executed on smart contract: "listAUs" index+1 represents AU_ID.
std::atomic<int> currAU;       //! used by miner-thread to get index of Atomic Unit to execute.
std::atomic<int> gNodeCount;   //! # of valid AU node added in graph (invalid AUs will not be part of the graph & conflict list).
std::atomic<int> eAUCount;     //! used by validator threads to keep track of how many valid AUs executed by validator threads.
std::atomic<int> *mAUT = NULL; //! array to map AUs to Trans id (time stamp); mAUT[index] = TransID, index+1 = AU_ID.
std::atomic<int>*status;       //! used by pool threads:: -1 = thread join; 0 = wait; 1 = execute AUs given in ref[].
Graph::Graph_Node **Gref;      //! used by pool threads:: graph node (AU) reference to be execute by respective Pool thread.


/*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
!    Class "Miner" CREATE & RUN "n" miner-THREAD CONCURRENTLY           !
!"concMiner()" CALLED BY miner-THREAD TO PERFROM oprs of RESPECTIVE AUs !
! THREAD 0 IS CONSIDERED AS MINTER-THREAD (SMART CONTRACT DEPLOYER)     !
!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
class Miner
{
	public:
	Miner( )
	{
		cGraph = NULL;
		cGraph = new Graph();
		//! initialize the counter used to execute the numAUs to
		//! 0, and graph node counter to 0 (number of AUs added
		//! in graph, invalid AUs will not be part of the grpah).
		currAU     = 0;
		gNodeCount = 0;

		//! index location represents respective thread id.
		mTTime = new float_t [nThread];
		gTtime = new float_t [nThread];
		aCount = new int [nThread];

		for(int i = 0; i < nThread; i++) 
		{
			mTTime[i] = 0;
			gTtime[i] = 0;
			aCount[i] = 0;
		}
		auction = new SimpleAuction(bidEndT, beneficiary, nBidder);
	}

	//!-------------------------------------------- 
	//!!!!!! MAIN MINER:: CREATE MINER THREADS !!!!
	//!--------------------------------------------
	void mainMiner()
	{
		Timer mTimer;
		thread T[nThread];

		//!------------------------------------------
		//!!!!!! CREATE nThreads MINER THREADS  !!!!!
		//!------------------------------------------
//		cout<<"!!!!!!!   Coarse-Grain Miner Thread Started   !!!!!!\n";
		//! Start clock.
		double start = mTimer.timeReq();
		for(int i = 0; i < nThread; i++)
		{
			T[i] = thread(concMiner, i, numAUs, cGraph);
		}
		//! miner thread join.
		for(auto& th : T)
		{
			th.join();
		}
		//! Stop clock.
		totalT[0] = mTimer.timeReq() - start;
//		cout<<"!!!!!!!   Coarse-Grain Miner Thread Join   !!!!!!\n";
	

		//! print conflict grpah.
//		cGraph->print_grpah();

		//! print the final state of the shared objects.
//		finalState();
		cout<<"\n Number of Valid   AUs = "+to_string(gNodeCount)
				+" (Grpah Nodes:: AUs Executed Successfully)\n";
		cout<<" Number of Invalid AUs = "+to_string(numAUs-gNodeCount)+"\n";
		
		 auction->AuctionEnded_m( );
	}

	//!--------------------------------------------------------
	//! The function to be executed by all the miner threads. !
	//!--------------------------------------------------------
	static void concMiner( int t_ID, int numAUs, Graph *cGraph)
	{
		Timer thTimer;

		//! flag is used to add valid AUs in Graph.
		//! (invalid AU: senders doesn't have sufficient balance to send).
		bool flag = true;

		//! get the current index, and increment it.
		int curInd = currAU++;

		//! statrt clock to get time taken by this.AU
		auto start = thTimer._timeStart();

		while( curInd < numAUs )
		{
			//! trns_id of STM_BTO_trans that successfully executed this AU.
			int t_stamp;

			//! trans_ids with which this AU.trans_id is conflicting.
			list<int>conf_list;
			conf_list.clear();
			//!get the AU to execute, which is of string type.
			istringstream ss(listAUs[curInd]);

			string tmp;
			//! AU_ID to Execute.
			ss >> tmp;

			int AU_ID = stoi(tmp);

			//! Function Name (smart contract).
			ss >> tmp;
			if(tmp.compare("bid") == 0)
			{
				ss >> tmp;
				int payable = stoi(tmp);//! payable
				ss >> tmp;
				int bID = stoi(tmp);//! Bidder ID
				ss >> tmp;
				int bAmt = stoi(tmp);//! Bidder value
				int v = auction->bid_m(payable, bID, bAmt,
										 &t_stamp, conf_list);
				while(v != 1)
				{
					aCount[0]++;
					v = auction->bid_m(payable, bID, bAmt, 
											&t_stamp, conf_list);
					if(v == -1)
					{
						flag = false;//! invalid AU.
						break;                                    
					}
				}
			}
			if(tmp.compare("withdraw") == 0)
			{
				ss >> tmp;
				int bID = stoi(tmp);//! Bidder ID

				int v = auction->withdraw_m(bID, &t_stamp, conf_list);
				while(v != 1)
				{
					aCount[0]++;
					v = auction->withdraw_m(bID, &t_stamp, conf_list);
					if(v == -1)
					{
						flag = false;//! invalid AU.
						break;                                    
					}
				}
			}
			if(tmp.compare("auction_end") == 0)
			{
				int v = auction->auction_end_m(&t_stamp, conf_list);
				while(v != 1)
				{
					aCount[0]++;
					v = auction->auction_end_m(&t_stamp, conf_list);
					if(v == -1)
					{
						flag = false;//! invalid AU.
						break;                                    
					}
				}
			}
	
			//! graph construction for committed AUs.
			if (flag == true) 
			{
				mAUT[AU_ID-1] = t_stamp;
				
				//! increase graph node counter
				//! (Valid AU executed).
				gNodeCount++;
				//! get respective trans 
				//! conflict list using lib fun
				//list<int>conf_list = lib->get_conf(t_stamp);
				
				//!::::::::::::::::::::::::::::::::::
				//! Remove all the time stamps from :
				//! conflict list, added because    :
				//! of initilization and creation   :
				//! of shared object in STM memory. :
				//!::::::::::::::::::::::::::::::::::
				for(int y = 0; y <= nBidder; y++)
					conf_list.remove(y);
				
				//! statrt clock to get time taken by this.thread 
				//! to add edges and node to conflict grpah.
				auto gstart = thTimer._timeStart();
				
				//!------------------------------------------
				//! conf_list come from contract fun using  !
				//! pass by argument of vote() & delegate() !
				//!------------------------------------------
				//! if AU_ID conflict is empty, 
				//! add node this AU_ID vertex node.
				if(conf_list.begin() == conf_list.end())
				{
					cGraph->gLock.lock();
					Graph::Graph_Node *tempRef;
					cGraph->add_node(AU_ID, t_stamp, &tempRef);
					cGraph->gLock.unlock();
				}
				for(auto it = conf_list.begin(); it != conf_list.end(); it++)
				{
					int i = 0;
					//! get conf AU_ID in map table 
					//! given conflicting tStamp.
					while(*it != mAUT[i]) i = (i+1)%numAUs;

					//! index start with 0 => 
					//! index+1 respresent AU_ID.
					//! cAUID = index+1, cTstamp 
					//! = mAUT[i] with this.AU_ID
					int cAUID   = i+1;
					int cTstamp = mAUT[i];

					if(cTstamp < t_stamp)
					{
						cGraph->gLock.lock();
						//! edge from cAUID to AU_ID.
						cGraph->add_edge(cAUID, AU_ID, cTstamp, t_stamp);
						cGraph->gLock.unlock();
					}
					if(cTstamp > t_stamp) 
					{
						cGraph->gLock.lock();
						//! edge from AU_ID to cAUID.
						cGraph->add_edge(AU_ID, cAUID, t_stamp, cTstamp);
						cGraph->gLock.unlock();
					}
				}
				gTtime[t_ID] = gTtime[t_ID] + thTimer._timeStop(gstart);
			}
			//! reset flag for next AU.
			flag = true;
			//! get the current index to execute, and increment it.
			curInd = currAU++;
			conf_list.clear();
		}
		mTTime[t_ID] = mTTime[t_ID] + thTimer._timeStop(start);
	}

	//!-------------------------------------------------
	//!FINAL STATE OF ALL THE SHARED OBJECT. Once all  |
	//!AUs executed. we are geting this using state_m()|
	//!-------------------------------------------------
	void finalState()
	{	
//		auction->state_m( );//Intire state
	}

	~Miner() { };
};




/*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
! Class "Validator" CREATE & RUN "n" validator-THREAD   !
! CONCURRENTLY BASED ON CONFLICT GRPAH GIVEN BY MINER.  !
! "concValidator()" CALLED BY validator-THREAD TO       !
! PERFROM OPERATIONS of RESPECTIVE AUs.  THREAD 0 IS    !
! CONSIDERED AS MINTER-THREAD (SMART CONTRACT DEPLOYER) !
!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
class Validator
{
public:

	Validator( )
	{
		//! int the execution counter used by validator threads.
		eAUCount = 0;
		
		status = new std::atomic<int>[nThread+1];
		Gref   = new Graph::Graph_Node*[nThread+1];

		//! array index location represents respective thread id.
		vTTime = new float_t [nThread];
		for(int i = 0; i <= nThread; i++)
		{
			status[i] = 0;
			Gref[i]   = NULL;
			vTTime[i] = 0;
		}
	}


	//!----------------------------------------------------------
	//! CREATE MASTER VALIDATOR THREADS: WHICH CREATES n WORKER !
	//! THREAD TO EXECUTE VALID AUs IN CONFLICT GRAPH.          !
	//!----------------------------------------------------------
	void mainValidator()
	{
		Timer vTimer;
		auction->reset();

		//!----------------------------------------------------
		//! INTIALIZE REQUIRED INFO AND CREATE MASTER THREAD. !
		//! MASTER THREAD CREATE n VALIDATOR THREADS          !
		//!----------------------------------------------------
//		cout<<"!!!!!!!   Coarse-Grain Validator Thread Started     !!!!!!\n";
		//!Start clock
		double start = vTimer.timeReq();
		
		thread master = thread(concValidator, 0 );
		master.join();
		
		//!Stop clock
		totalT[1] = vTimer.timeReq() - start;
//		cout<<"!!!!!!!   Coarse-Grain Validator Thread Join        !!!!!!\n\n";

		//!print the final state of the shared objects by validator.
//		finalState();
		auction->AuctionEnded( );
	}

	//!-------------------------------------------------------
	//! FUNCTION TO BE EXECUTED BY ALL THE VALIDATOR THREADS.!
	//!-------------------------------------------------------
	static void concValidator( int t_ID )
	{
		Timer thTimer;

		//!statrt clock to get time taken by this thread.
		auto start = thTimer._timeStart();

		//! ONLY MASTER THREAD WILL EXECUTE IT.
		if(t_ID == 0)
		{
			thread POOL[nThread+1];
			bool tCratFlag = true;//! POOL thread creation flag.
			Graph::Graph_Node *mVItr;
			while(true)
			{
				if(tCratFlag == true)
				{
					//! Creating n POOL Threads
					for(int i = 1; i <= nThread; i++)
					{
						POOL[i] = thread(concValidator, i);
					}
					tCratFlag = false;
				}
				//! All Valid AUs executed.
				if(eAUCount == gNodeCount)
				{
					for(int i = 1; i <= nThread; i++)
					{
						//! -1 = threads can join now.
						status[i] = -1;
					}
					
					//! POOL thread join.
					for(int i = 1; i <= nThread; i++)
					{
						POOL[i].join( );
					}
					break;
				}
				mVItr = cGraph->verHead->next;
				while(mVItr != cGraph->verTail)
				{
					if(mVItr->in_count == 0)
					{
						for(int i = 1; i <= nThread; i++)
						{
							//! 0 = thread is available.
							if(status[i] == 0)
							{
								//! assigning node ref for
								//! thread in pool to execute.
								Gref[i] = mVItr;
								//! 1 = ref is available to execute.
								status[i] = 1;
								break;
							}
						}
					}
					mVItr = mVItr->next;
				}
			}
		}
		//! EXECCUTED BY nThread WORKER THREADS.
		else
		{
			while(true)
			{
				if(status[t_ID] == -1 || eAUCount == gNodeCount) 
					break;//! All task done.
				
				if(status[t_ID] == 1)//! Task available to work on.
				{
					Graph::Graph_Node *verTemp;
					verTemp      = Gref[t_ID];
					if(verTemp->in_count == 0)
					{
						cGraph->gLock.lock();
						if(verTemp->in_count < 0)
						{
							cGraph->gLock.unlock();
							status[t_ID] = 0;
						}
						else
						{
							verTemp->in_count = -1;
							cGraph->gLock.unlock();

							//! get AU to execute, which is of string type;
							//! listAUs index statrt with 0 => -1.
							istringstream ss(listAUs[(verTemp->AU_ID)-1]);
							string tmp;

							//! AU_ID to Execute.
							ss >> tmp;
							int AU_ID = stoi(tmp);

							//! Function Name (smart contract).
							ss >> tmp;
							if(tmp.compare("bid") == 0)
							{
								ss >> tmp;
								int payable = stoi(tmp);//! payable
								ss >> tmp;
								int bID = stoi(tmp);//! Bidder ID
								ss >> tmp;
								int bAmt = stoi(tmp);//! Bidder value
								bool v = auction->bid(payable, bID, bAmt);
							}
							if(tmp.compare("withdraw") == 0)
							{
								ss >> tmp;
								int bID = stoi(tmp);//! Bidder ID
								bool v = auction->withdraw(bID);
							}
							if(tmp.compare("auction_end") == 0)
							{
								bool v = auction->auction_end( );
							}

							Graph::EdgeNode *eTemp = verTemp->edgeHead->next;
							while( eTemp != verTemp->edgeTail)
							{
								Graph::Graph_Node* refVN = 
									(Graph::Graph_Node*)eTemp->ref;
								refVN->in_count--;
								eTemp = eTemp->next;
							}
							//! num of Valid AUs executed is eAUCount+1.
							eAUCount++;
						}
					}
					//Gref[t_ID]   = NULL;
					status[t_ID] = 0;
				}
			}
		}



		vTTime[t_ID] = vTTime[t_ID] + thTimer._timeStop(start);
	}


	//!-------------------------------------------------
	//!FINAL STATE OF ALL THE SHARED OBJECT. ONCE ALL  |
	//!AUS EXECUTED. WE ARE GETING THIS USING state()  |
	//!-------------------------------------------------
	void finalState()
	{	
//		auction->state( );//Intire state
	}
	~Validator() { };
};




/*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
/*!!!!!!!!!!!!!!!   main() !!!!!!!!!!!!!!!!!!*/
/*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
int main( )
{
	//! list holds the avg time 
	//! taken by miner and Validator
	//! thread s for multiple 
	//! consecutive runs.
	list<double>mItrT;
	list<double>vItrT;

	FILEOPR file_opr;

	//! read from input file:: nBidder = 
	//! #numProposal; nThread = #threads;
	//! numAUs = #AUs; λ = random delay seed.
	file_opr.getInp(&nBidder, &bidEndT, &nThread, &numAUs, &lemda);
		

	//! max Proposal shared object error handling.
	if(nBidder > maxBObj) 
	{
		nBidder = maxBObj;
		cout<<"Max number of Proposal Shared Object can be "<<maxBObj<<"\n";
	}

	cout<<pl<<" MVTO/Fork-Join/Coarse Grain Algorithm \n"<<pl;
	int totalRun = 1;
	for(int numItr = 0; numItr < totalRun; numItr++)
	{
		
		//! index+1 represents respective AU id, and
		//! mAUT[index] represents "time stamp (commited trans)".
		mAUT = new std::atomic<int>[numAUs];
		for(int i = 0; i< numAUs; i++)
			mAUT[i] = 0;
		
		
		//! generates AUs (i.e. trans to be executed by miner & validator).
		file_opr.genAUs(numAUs, nBidder, funInContract, listAUs);

		//MINER
		Miner *miner = new Miner();
		miner ->mainMiner();

		//VALIDATOR
		Validator *validator = new Validator( );
		validator ->mainValidator();

		float_t gConstT = 0;
		for(int ii = 0; ii < nThread; ii++)
		{
			gConstT += gTtime[ii];
		}
		cout<<"Avg Grpah Time= "<<gConstT/nThread<<" microseconds";
		
		//! total valid AUs among total AUs executed 
		//! by miner and varified by Validator.
		int vAUs = numAUs-aCount[0];
		
		file_opr.writeOpt(nBidder, nThread, numAUs, 
							totalT, mTTime, vTTime, 
							aCount, vAUs, mItrT, vItrT);
		cout<<"\n===================== Execution "
			<<numItr+1<<" Over =====================\n"<<endl;

		listAUs.clear();
		delete miner;
		miner = NULL;
		delete validator;
		validator = NULL;
		delete cGraph;
		cGraph = NULL;
		delete auction;
		auction = NULL;
	}
	
	//! To get total avg miner and validator
	//! time after number of totalRun runs.
	double tAvgMinerT = 0, tAvgValidT = 0;
	auto mit = mItrT.begin();
	auto vit = vItrT.begin();
	for(int j = 0; j < totalRun; j++)
	{
		tAvgMinerT = tAvgMinerT + *mit;
//		cout<<"\n    Avg Miner = "<<*mit;
		tAvgValidT = tAvgValidT + *vit;
//		cout<<"\nAvg Validator = "<<*vit;
		mit++;
		vit++;
	}
	tAvgMinerT = tAvgMinerT/totalRun;
	tAvgValidT = tAvgValidT/totalRun;
	cout<<pl<<"    Total Avg Miner = "<<tAvgMinerT<<" microseconds";
	cout<<"\nTotal Avg Validator = "<<tAvgValidT<<" microseconds";
	cout<<"\n  Total Avg (M + V) = "<<tAvgMinerT+tAvgValidT<<" microseconds";
	cout<<"\n"<<pl;

	mItrT.clear();
	vItrT.clear();
return 0;
}
