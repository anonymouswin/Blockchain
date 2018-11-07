/********************************************************************
|  *****    Ballot Contract BTO Miner Validator              *****  |
|  *****     File:   coarse.cpp                              *****  |
|  *****  Created:   Aug 18, 2018, 11:46 AM                  *****  |
*********************************************************************
|  *****  COMPILE:: g++ coarse.cpp -pthread -std=c++14 -ltbb *****  |
********************************************************************/

#include <iostream>
#include <thread>
#include "Util/Timer.cpp"
#include "Contract/Ballot.cpp"
#include "Graph/Coarse/Graph.cpp"
#include "Util/FILEOPR.cpp"

#define maxThreads 128
#define maxPObj 100
#define maxVObj 4000
#define funInContract 5
#define pl "=============================\n"

using namespace std;
using namespace std::chrono;

int    nProposal = 2;      //! nProposal: number of proposal shared objects;
int    nVoter    = 1;      //! nVoter: number of voter shared objects;
int    nThread   = 1;      //! nThread: total number of concurrent threads; default is 1.
int    numAUs;             //! numAUs: total number of Atomic Unites to be executed.
double lemda;              //! λ: random delay seed.

double totalTime[2];       //! total time taken by miner and validator algorithm.
Ballot *ballot;            //! smart contract.
Graph  *cGraph;            //! conflict grpah generated by miner to be given to validator.
int    *aCount;            //! aborted transaction count.
float_t *mTTime;           //! time taken by each miner Thread to execute AUs (Transactions).
float_t *vTTime;           //! time taken by each validator Thread to execute AUs (Transactions).
float_t *gTtime;           //! time taken by each miner Thread to add edges and nodes in the conflict graph.
vector<string>listAUs;     //! holds AUs to be executed on smart contract: "listAUs" index+1 represents AU_ID.
std::atomic<int>currAU;    //! used by miner-thread to get index of Atomic Unit to execute.
std::atomic<int>gNodeCount;//! # of valid AU node added in graph (invalid AUs will not be part of the graph & conflict list).
std::atomic<int>eAUCount;  //! used by validator threads to keep track of how many valid AUs executed by validator threads.
std::atomic<int>*mAUT;     //! array to map AUs to Trans id (time stamp); mAUT[index] = TransID, index+1 = AU_ID.

string *proposalNames;

/*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
!    Class "Miner" CREATE & RUN "n" miner-THREAD CONCURRENTLY           !
!"concMiner()" CALLED BY miner-THREAD TO PERFROM oprs of RESPECTIVE AUs !
! THREAD 0 IS CONSIDERED AS MINTER-THREAD (SMART CONTRACT DEPLOYER)     !
!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
class Miner
{
	public:
	Miner(int chairperson)
	{
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
		
		proposalNames = new string[nProposal+1];
		for(int x = 0; x <= nProposal; x++)
		{
			proposalNames[x] = "X"+to_string(x+1);
		}
		for(int i = 0; i < nThread; i++) 
		{
			mTTime[i] = 0;
			gTtime[i] = 0;
			aCount[i] = 0;
		}
		
		//! Id of the contract creater is \chairperson = 0\.
		ballot = new Ballot( proposalNames, chairperson, nVoter, nProposal);
	}

	//!------------------------------------------------------------------------- 
	//!!!!!!!! MAIN MINER:: CREATE MINER + GRAPH CONSTRUCTION THREADS !!!!!!!!!!
	//!-------------------------------------------------------------------------
	void mainMiner()
	{
		Timer mTimer;
		thread T[nThread];

		//! Give \`voter\` the right to vote on this ballot.
		//! giveRightToVote_m() is serial.
		for(int voter = 1; voter <= nVoter; voter++) 
		{
			//! 0 is chairperson.
			ballot->giveRightToVote_m(0, voter);
		}

		//!-----------------------------------------------------------
		//!!!!!!!!!!    Create nThread Miner threads      !!!!!!!!!!
		//!-----------------------------------------------------------
//		cout<<"!!!!!!!   Coarse-Grain Miner Thread Started         !!!!!!\n";
		//! Start clock to get time taken by miner algorithm.
		double start = mTimer.timeReq();
		for(int i = 0; i < nThread; i++)
		{
			T[i] = thread(concMiner, i, numAUs, cGraph);
		}
		//! miner thread join
		for(auto& th : T)
		{
			th.join();
		}
		//! Stop clock
		totalTime[0] = mTimer.timeReq() - start;
//		cout<<"!!!!!!!   Coarse-Grain Miner Thread Join            !!!!!!\n";
	

		//! print conflict grpah.
//		cGraph->print_grpah();

		//! print AU_ID and Timestamp.
//		FILEOPR file_opr;
//		file_opr.pAUTrns(mAUT, numAUs);

		//! print the final state of the shared objects.
//		finalState();
//		ballot->winningProposal_m();
		string winner;
		ballot->winnerName_m(&winner);
//		cout<<"\n Number of Valid   AUs = "+to_string(gNodeCount)
//				+" (Grpah Nodes:: AUs Executed Successfully)\n";
//		cout<<" Number of Invalid AUs = "+to_string(numAUs-gNodeCount)+"\n";
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

			if(tmp.compare("vote") == 0)
			{
				ss >> tmp;
				int vID = stoi(tmp);//! voter ID
				ss >> tmp;
				int pID = stoi(tmp);//! proposal ID
				int v = ballot->vote_m(vID, pID, &t_stamp, conf_list);
				while( v != 1 )
				{
					aCount[t_ID]++;
					v = ballot->vote_m(vID, pID, &t_stamp, conf_list);					
					if(v == -1)
					{
						//! invalid AU:sender does't have sufficent bal to send.
						flag = false;
						break;                                    
					}
				}
			}
			if(tmp.compare("delegate") == 0)
			{
				ss >> tmp;
				int sID = stoi(tmp);//! Sender ID
				ss >> tmp;
				int rID = stoi(tmp);//! Reciver ID

				//! execute again if tryCommit fails
				int v = ballot->delegate_m(sID, rID, &t_stamp, conf_list);	
				while( v != 1 )
				{
					aCount[t_ID]++;
					v = ballot->delegate_m(sID, rID, &t_stamp, conf_list);
					if(v == -1)
					{
						//! invalid AU:sender does't have sufficent bal to send.
						flag = false;
						break;                                    
					}
				}
			}
			//! graph construction for committed AUs.
			if (flag == true) 
			{
				mAUT[AU_ID-1] = t_stamp;
				gNodeCount++;//! increase graph node counter (Valid AU executed)
				//! get respective trans conflict list using lib fun
				//list<int>conf_list = lib->get_conf(t_stamp);
				
				//!::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
				//! Remove all the time stamps from conflict list, added because 
				//! of initilization and creation of shared object in STM memory.
				//!::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
				for(int y = 1; y <= (2*nVoter+nProposal+1); y++)
					conf_list.remove(y);
				
				//! statrt clock to get time taken by this.thread 
				//! to add edges and node to conflict grpah.
				auto gstart = thTimer._timeStart();
				
				//!------------------------------------------
				//! conf_list come from contract fun using  !
				//! pass by argument of vote() & delegate() !
				//!------------------------------------------
				//! if AU_ID conflict is empty, add node this AU_ID vertex node.
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
					//! get conf AU_ID in map table given conflicting tStamp.
					while(*it != mAUT[i]) i = (i+1)%numAUs;

					//! index start with 0 => index+1 respresent AU_ID.
					//! cAUID = index+1, cTstamp = mAUT[i] with this.AU_ID
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
		for(int id = 1; id <= nVoter; id++) 
		{
//			ballot->state_m(id, true);//for voter state
		}
		for(int id = 1; id <= nProposal; id++) 
		{
			ballot->state_m(id, false);//for voter state
		}
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

	Validator()
	{
		//! int the execution counter used by validator threads.
		eAUCount = 0;
		
		//! array index location represents respective thread id.
		vTTime = new float_t [nThread];
		for(int i = 0; i < nThread; i++)
		{ 
			vTTime[i] = 0;
		}
	};

	/*!---------------------------------------
	| create n concurrent validator threads  |
	| to execute valid AUs in conflict graph.|
	----------------------------------------*/
	void mainValidator()
	{
		Timer vTimer;
		thread T[nThread];

		//! giveRightToVote() function is serial.
		for(int voter = 1; voter <= nVoter; voter++) 
		{
			//! 0 is chairperson.
			ballot->giveRightToVote(0, voter);
		}

		//!------------------------------_-----------
		//!!!!! Create nThread Validator threads !!!!
		//!------------------------------------------
//		cout<<"!!!!!!!   Coarse-Grain Validator Thread Started     !!!!!!\n";
		//!Start clock
		double start = vTimer.timeReq();
		for(int i = 0; i<nThread; i++)
		{
			T[i] = thread(concValidator, i);
		}
		//!validator thread join.
		for(auto& th : T)
		{
			th.join( );
		}
		//!Stop clock
		totalTime[1] = vTimer.timeReq() - start;
//		cout<<"!!!!!!!   Coarse-Grain Validator Thread Join        !!!!!!\n\n";

		//!print the final state of the shared objects by validator.
//		finalState();
//		ballot->winningProposal();
		string winner;
		ballot->winnerName(&winner);
	}

	//!-------------------------------------------------------
	//! Function to be executed by all the validator threads.!
	//!-------------------------------------------------------
	static void concValidator( int t_ID )
	{
		Timer thTimer;

		//!statrt clock to get time taken by this thread.
		auto start = thTimer._timeStart();
		
		list<Graph::Graph_Node*>buffer;
		auto itr = buffer.begin();

		Graph::Graph_Node *verTemp;
		while( true )
		{
			//! uncomment this to remove effect of local buffer optimization.
			//buffer.clear();

			//! all Graph Nodes (Valid AUs executed)
			if(eAUCount == gNodeCount ) break;
			
			//!-----------------------------------------
			//!!!<< AU execution from local buffer. >>!!
			//!-----------------------------------------
			for(itr = buffer.begin(); itr != buffer.end(); itr++)
			{
				Graph::Graph_Node* temp = *itr;
				if(temp->in_count == 0)
				{
					//! get lock X1.
					cGraph->gLock.lock();

					//! AU is unclaimed if in_count ==> 0
					//! unexecuted if marked ==> false
					if(temp->marked == false )
					{
						//! AU claimed by this.t_ID
						temp->marked = true;

						//! logical delete, node will not be executed
						//! again::even marked field is logical delete.
						temp->in_count = -1;

						//! num of Valid AUs executed is eAUCount+1.
						eAUCount++;

						//! relase lock X1 either here or in else part.
						cGraph->gLock.unlock();

						//! get AU to execute, which is of string type;
						//! listAUs index statrt with 0 ==> -1.
						istringstream ss( listAUs[(temp->AU_ID) - 1]);
						string tmp;

						//! AU_ID to Execute.
						ss >> tmp;
						int AU_ID = stoi(tmp);


						//! Function Name (smart contract).
						ss >> tmp;
						if(tmp.compare("vote") == 0)
						{
							ss >> tmp;
							int vID = stoi(tmp);//! voter ID
							ss >> tmp;
							int pID = stoi(tmp);//! proposal ID
							int v = ballot->vote(vID, pID);
						}
						if(tmp.compare("delegate") == 0)
						{
							ss >> tmp;
							int sID = stoi(tmp);//! Sender ID
							ss >> tmp;
							int rID = stoi(tmp);//! Reciver ID
							int v = ballot->delegate(sID, rID);
						}

						Graph::EdgeNode *eTemp = temp->edgeHead->next;
						while( eTemp != temp->edgeTail)
						{
							Graph::Graph_Node* refVN = (Graph::Graph_Node*)eTemp->ref;

							refVN->in_count--;
							if(refVN->in_count == 0 )
							{
								//! insert into local buffer.
								buffer.push_back(refVN);
							}
							eTemp = eTemp->next;
						}
						delete eTemp;
					}
					else
					{
						cGraph->gLock.unlock();
					}
				}
			}
			//! reached to end of local buffer; clear the buffer.
			buffer.clear();

			//!-----------------------------------------------------
			//!!!<< AU execution by traversing conflict grpah  >>!!!
			//!-----------------------------------------------------
			verTemp = cGraph->verHead->next;
			while(verTemp != cGraph->verTail)
			{
				if(verTemp->in_count == 0)
				{
					//! get lock X1.
					cGraph->gLock.lock();

					//! unclaimed unexecuted AU.
					if(verTemp->marked == false )
					{
						//! AU claimed by this.t_ID
						verTemp->marked   = true;

						//! this node will not be executed again:: 
						//! marked field/in_count is logical delete.
						verTemp->in_count = -1;

						//! num of Valid AUs executed is eAUCount+1.
						eAUCount++;

						//!relase lock X1 either here or at else condition at end.
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
						if(tmp.compare("vote") == 0)
						{
							ss >> tmp;
							int vID = stoi(tmp);//! voter ID
							ss >> tmp;
							int pID = stoi(tmp);//! proposal ID
							int v = ballot->vote(vID, pID);
						}
						if(tmp.compare("delegate") == 0)
						{
							ss >> tmp;
							int sID = stoi(tmp);//! Sender ID
							ss >> tmp;
							int rID = stoi(tmp);//! Reciver ID
							int v = ballot->delegate(sID, rID);
						}
						Graph::EdgeNode *eTemp = verTemp->edgeHead->next;
						while( eTemp != verTemp->edgeTail)
						{
							Graph::Graph_Node* refVN = (Graph::Graph_Node*)eTemp->ref;
							refVN->in_count--;
							if(refVN->in_count == 0 )
							{
								//! insert into local buffer.
								buffer.push_back( refVN );
							}
							eTemp = eTemp->next;
						}
					}
					else
					{
						cGraph->gLock.unlock(); //! or relase lock X1 here.
					}
				}
				verTemp = verTemp->next;
			}
		}
		verTemp = NULL;
		delete verTemp;
		buffer.clear();
		vTTime[t_ID] = vTTime[t_ID] + thTimer._timeStop(start);
	}


	//!-------------------------------------------------
	//!FINAL STATE OF ALL THE SHARED OBJECT. Once all  |
	//!AUs executed. we are geting this using get_bel()|
	//!-------------------------------------------------
	void finalState()
	{
		for(int id = 1; id <= nVoter; id++) 
		{
//			ballot->state(id, true);//for voter state
		}
		for(int id = 1; id <= nProposal; id++) 
		{
			ballot->state(id, false);//for voter state
		}
	}
	~Validator() { };
};




/*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
/*!!!!!!!!!!!!!!!   main() !!!!!!!!!!!!!!!!!!*/
/*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
int main( )
{
	//! list holds the avg time taken by miner and Validator
	//! thread s for multiple consecutive runs.
	list<double>mItrT;
	list<double>vItrT;

	cout<<pl<<" Coarse Grain Algorithm \n"<<pl;
	int totalRun = 1;
	for(int numItr = 0; numItr < totalRun; numItr++)
	{
		FILEOPR file_opr;

		//! read from input file:: nProposal = #numProposal; nThread = #threads;
		//! numAUs = #AUs; λ = random delay seed.
		file_opr.getInp(&nProposal, &nVoter, &nThread, &numAUs, &lemda);

		//! max Proposal shared object error handling.
		if(nProposal > maxPObj) 
		{
			nProposal = maxPObj;
			cout<<"Max number of Proposal Shared Object can be "<<maxPObj<<"\n";
		}
		
		//! max Voter shared object error handling.
		if(nVoter > maxVObj) 
		{
			nVoter = maxVObj;
			cout<<"Max number of Proposal Shared Object can be "<<maxVObj<<"\n";
		}
		 //! generates AUs (i.e. trans to be executed by miner & validator).
		file_opr.genAUs(numAUs, nVoter, nProposal, funInContract, listAUs);
		
		//! index+1 represents respective AU id, and
		//! mAUT[index] represents "time stamp (commited trans)".
		mAUT = new std::atomic<int>[numAUs];
		for(int i = 0; i< numAUs; i++)
		{
			mAUT[i] = 0;
		}
		Timer mTimer;
		mTimer.start();

		//MINER
		Miner *miner = new Miner(0);
		miner ->mainMiner();

		//VALIDATOR
		Validator *validator = new Validator();
		validator ->mainValidator();

		mTimer.stop();

		float_t gConstT = 0;
		for(int ii = 0; ii < nThread; ii++)
		{
			gConstT += gTtime[ii];
		}
		cout<<"Avg Grpah Time= "<<gConstT/nThread<<" microseconds";

		//! total valid AUs among total AUs executed 
		//! by miner and varified by Validator.
		int vAUs = gNodeCount;
		file_opr.writeOpt(nProposal, nVoter, nThread, numAUs, 
							totalTime, mTTime, vTTime, aCount, vAUs, mItrT, vItrT);

//		cout<<"\n\nc CPU time:  " << main_timer->cpu_ms_total() <<" ms \n";
//		cout<<"c Real time: "    << main_timer->real_ms_total()<<" ms \n";
		cout<<"\n===================== Execution "<<numItr+1
			<<" Over =====================\n"<<endl;

		listAUs.clear();
		delete miner;
		delete cGraph;
		delete validator;
	}
	//! to get total avg miner and validator
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
