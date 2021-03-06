/********************************************************************
|  *****      Mix Contract MVTO Miner + Fork-Join Validator  *****  |
|  *****     File:   fine.cpp                                *****  |
|  *****  Created:   Oct 05, 2018, 11:46 AM                  *****  |
*********************************************************************
|  *****  COMPILE:: g++ fine.cpp -pthread -std=c++14 -ltbb   *****  |
********************************************************************/

#include <iostream>
#include <thread>
#include <atomic>
#include <list>
#include "Util/Timer.cpp"
#include "Util/FILEOPR.cpp"
#include "Contract/Mix.cpp"
#include "Graph/Fine/Graph.cpp"

#define MAX_THREADS 128
#define maxCoinAObj 1000  //maximum coin contract \account\ object.
#define CFCount 3         //# methods in coin contract.

#define maxPropObj 100    //maximum ballot contract \proposal\ shared object.
#define maxVoterObj 1000  //maximum ballot contract \voter\ shared object.
#define BFCount 5         //# methods in ballot contract.

#define maxBidrObj 1000   //maximum simple auction contract \bidder\ shared object.
#define maxbEndT 100      //maximum simple auction contract \bidding time out\ duration.
#define AFConut 6         //# methods in simple auction contract.
#define pl "==========================\n"

using namespace std;
using namespace std::chrono;

//! INPUT FROM INPUT FILE.
int nThread;    //! # of concurrent threads;
int numAUs;     //! # Atomic Unites to be executed.
int CoinSObj;   //! # Coin Shared Object.
int nProposal;  //! # Ballot Proposal Shared Object.
int nVoter;     //! # Ballot Voter Shared Object.
int nBidder;    //! # Ballot Voter Shared Object.
int bidEndT;    //! time duration for auction.

double tTime[2];           //! total time taken by miner & validator.
int    *aCount;            //! Invalid transaction count.
Graph  *cGraph;            //! conflict grpah generated by miner to be given to validator.
float_t *gTtime;           //! time taken by each miner Thread to add edges and nodes in the conflict graph.
float_t*mTTime;            //! time taken by each miner Thread to execute AUs.
float_t*vTTime;            //! time taken by each validator Thread to execute AUs.
vector<string>listAUs;     //! AUs to be executed on contracts: index+1 => AU_ID.
std::atomic<int>currAU;    //! used by miner-thread to get index of AU to execute.
std::atomic<int>gNodeCount;//! # of valid AU node added in graph (invalid AUs will not be part of the graph & conflict list).
std::atomic<int>eAUCount;  //! used by validator threads to keep track of how many valid AUs executed by validator threads.
std::atomic<int>*mAUT;     //! array to map AUs to Trans id (time stamp); mAUT[index] = TransID, index+1 = AU_ID.
string *proposalNames;     //! proposal names for ballot contract.

std::atomic<int>*status;   //! used by pool threads:: -1 = thread join; 0 = wait; 1 = execute AUs given in ref[].
Graph::Graph_Node **Gref;  //! used by pool threads:: graph node (AU) reference to be execute by respective Pool thread.


//! CONTRACT OBJECTS;
Coin   *coin;            //! coin contract miner object.
Ballot *ballot;          //! ballot contract for miner object.
SimpleAuction *auction;  //! simple auction contract for miner object.


/*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
!    Class "Miner" CREATE & RUN "n" miner-THREAD CONCURRENTLY           !
!"concMiner()" CALLED BY miner-THREAD TO PERFROM oprs of RESPECTIVE AUs !
! THREAD 0 IS CONSIDERED AS MINTER-THREAD (SMART CONTRACT DEPLOYER)     !
!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
class Miner
{
	public:
	Miner(int contDeployer)
	{
		cGraph = new Graph();

		//! initialize the counter used to execute the numAUs to.
		//! 0, and graph node counter to 0 (number of AUs added.
		//! in graph, invalid AUs will not part be of the grpah).
		currAU     = 0;
		gNodeCount = 0;

		proposalNames = new string[nProposal+1];
		for(int x = 0; x <= nProposal; x++)
		{
			proposalNames[x] = "X"+to_string(x+1);
		}
		
		
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
		//! id of the contract creater is "minter_id = 5000".
		coin = new Coin( CoinSObj, 5000 );
		
		//! Id of the contract creater is \chairperson = 10000\.
		ballot = new Ballot( proposalNames, 10000, nVoter, nProposal);
		
		//! fixed beneficiary id to 0, it can be any unique address/id.
		auction = new SimpleAuction(bidEndT, 0, nBidder);
	}

	//!---------------------------------------------------- 
	//!!!!!!!! MAIN MINER:: CREATE MINER THREADS !!!!!!!!!!
	//!----------------------------------------------------
	void mainMiner()
	{
		Timer mTimer;
		thread T[nThread];


		//! initialization of account with
		//! fixed ammount; mint_m() is serial.
		int ts, bal = 1000, total = 0;
		for(int sid = 1; sid <= CoinSObj; sid++) 
		{
			//! 5000 is contract deployer.
			coin->mint_m(5000, sid, bal, &ts);
			total = total + bal;
		}

		//! Give \`voter\` the right to vote on this ballot.
		//! giveRightToVote_m() is serial.
		for(int voter = 1; voter <= nVoter; voter++) 
		{
			//! 10000 is chairperson.
			ballot->giveRightToVote_m(10000, voter);
		}


		//!--------------------------------------------------------
		//!!!!!!!!!!    Create nThread Miner threads      !!!!!!!!!
		//!--------------------------------------------------------
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
		tTime[0] = mTimer.timeReq() - start;


		//! print conflict grpah.
//		cGraph->print_grpah();

		//! print the final state of the shared objects.
//		finalState();
//		string winner;
//		ballot->winnerName_m(&winner);
//		auction->AuctionEnded_m( );
	}

	//!------------------------------
	//! The function to be executed !
	//! by all the miner threads.   !
	//!------------------------------
	static void concMiner( int t_ID, int numAUs, Graph *cGraph)
	{
		Timer thTimer;

		//! flag is used to add valid AUs in Graph.
		//! (invalid AU: senders doesn't
		//! have sufficient balance to send).
		bool flag = true;

		//! get the current index, and increment it.
		int curInd = currAU++;

		//! statrt clock to get time taken by this.AU
		auto start = thTimer._timeStart();

		while( curInd < numAUs )
		{
			//! trns_id of STM_BTO_trans that
			//! successfully executed this AU.
			int t_stamp;

			//! trans_ids with which this
			//! AU.trans_id is conflicting.
			list<int>conf_list;
			conf_list.clear();
			//!get the AU to execute,
			//! which is of string type.
			istringstream ss(listAUs[curInd]);

			string tmp;
			//! AU_ID to Execute.
			ss >> tmp;

			int AU_ID = stoi(tmp);

			//! Function Name (smart contract).
			ss >> tmp;

			if(tmp.compare("get_bal") == 0)
			{
				//! get balance of SObj/id.
				ss >> tmp;
				int s_id = stoi(tmp);
				int bal  = 0;

				//! get_bal of smart contract::
				//! execute again if tryCommit fails.
				while(coin->get_bal_m(s_id, &bal, t_ID, 
							&t_stamp, conf_list) == false)
				{
					aCount[t_ID]++;
				}
			}

			if(tmp.compare("send") == 0)
			{
				//! Sender ID
				ss >> tmp;
				int s_id = stoi(tmp);

				//! Reciver ID
				ss >> tmp;
				int r_id = stoi(tmp);

				//! Ammount to send
				ss >> tmp;
				int amt  = stoi(tmp);
				int v = coin->send_m(t_ID, s_id, r_id, 
										amt, &t_stamp, conf_list);
				//! execute again if tryCommit fails
				while(v != 1 )
				{
					aCount[t_ID]++;
					v = coin->send_m(t_ID, s_id, r_id, 
										amt, &t_stamp, conf_list);
					if(v == -1)
					{
						//! invalid AU:sender does't 
						//! have sufficent bal to send.
						flag = false;
						break;                                    
					}
				}
			}
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
						//! invalid AU
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
						//! invalid AU:
						flag = false;
						break;                                    
					}
				}
			}
			if(tmp.compare("bid") == 0)
			{
				ss >> tmp;
				int payable = stoi(tmp);//! payable
				ss >> tmp;
				int bID = stoi(tmp);//! Bidder ID
				ss >> tmp;
				int bAmt = stoi(tmp);//! Bidder value
				int v = auction->bid_m(payable, bID, bAmt, &t_stamp, conf_list);
				while(v != 1)
				{
					aCount[0]++;
					v = auction->bid_m(payable, bID, bAmt, &t_stamp, conf_list);
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
				gNodeCount++;
				//! get respective trans conflict list using lib fun
				//list<int>conf_list = lib->get_conf(t_stamp);
				
				//!::::::::::::::::::::::::::::::::::
				//! Remove all the time stamps from :
				//! conflict list, added because    :
				//! of initilization and creation   :
				//! of shared object in STM memory. :
				//!::::::::::::::::::::::::::::::::::
				int TSRemove = CoinSObj+(2*nVoter+nProposal+1)+nBidder;
				for(int y = 1; y <= TSRemove; y++)
					conf_list.remove(y);
			
				//! statrt clock to get time taken by this.thread 
				//! to add edges and node to conflict grpah.
				auto gstart = thTimer._timeStart();
				
				//!------------------------------------------
				//! conf_list come from contract fun using  !
				//! pass by argument of get_bel() and send()!
				//!------------------------------------------
				//! when AU_ID conflict is empty.
				if(conf_list.begin() == conf_list.end())
				{
					Graph:: Graph_Node *tempRef;
					cGraph->add_node(AU_ID, t_stamp, &tempRef);
				}

				for(auto it = conf_list.begin(); it != conf_list.end(); it++)
				{
					int i = 0;
					//! find the conf_AU_ID 
					//! in map table given 
					//! conflicting time-stamp.
					while(*it != mAUT[i])
					i = (i+1)%numAUs; 

					//! because array index start 
					//! with 0 and index+1 
					//! respresent AU_ID.
					int cAUID   = i+1;
					
					//! conflicting AU_ID
					//! with this.AU_ID.
					int cTstamp = mAUT[i];
					
					 //! edge from conf_AU_ID to AU_ID.
					if(cTstamp  < t_stamp)
						cGraph->add_edge(cAUID, AU_ID, cTstamp, t_stamp);
					
					//! edge from AU_ID to conf_AU_ID.
					if(cTstamp > t_stamp)
						cGraph->add_edge(AU_ID, cAUID, t_stamp, cTstamp);
				}
				gTtime[t_ID] = gTtime[t_ID] + thTimer._timeStop(gstart);
			}
			//! reset flag for next AU.
			flag   = true;
			//! get the current index to 
			//! execute, and increment it.
			curInd = currAU++;
			conf_list.clear();
		}

		mTTime[t_ID] = mTTime[t_ID] + thTimer._timeStop(start);
	}

	//!-------------------------------
	//!FINAL STATE OF ALL THE SHARED |
	//!OBJECT. Once all AUs executed.|
	//!-------------------------------
	void finalState()
	{
		list<int>cList;	int total = 0;
		cout<<pl<<"SHARED OBJECTS FINAL STATE\n"<<pl
				<<"ACCT ID | FINAL STATE\n"<<pl;
		for(int sid = 1; sid <= CoinSObj; sid++) 
		{
			int bal = 0, ts;
			//! get_bal_m() of smart contract,
			//! execute again if tryCommit fails.
			while( coin->get_bal_m(sid, &bal, 0, &ts, cList) == false ) 
			{
				
			}
			cout<<"  "+to_string(sid)+"  \t|  "+to_string(bal)+"\n";
			total = total + bal;
			cList.clear();
		}
		cout<<pl<<"  SUM   |  "<<total<<"\n"<<pl;
		
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
		eAUCount = 0;
		 //! array index location represents respective thread id.
		vTTime   = new float_t[nThread];
	
		status = new std::atomic<int>[nThread+1];
		Gref   = new Graph::Graph_Node*[nThread+1];

		for(int i = 0; i < nThread; i++)
		{
			status[i] = 0;
			Gref[i]   = NULL;
			vTTime[i] = 0;	
		}
	};

	//!-------------------------------------------------
	//! CREATE MASTER THREADS: CREATES n WORKER        !
	//! THREADs TO EXECUTE VALID AUs IN CONFLICT GRAPH.!
	//!-------------------------------------------------
	void mainValidator()
	{
		Timer Ttimer;
		auction->reset();
		
		//! initialization of account with fixed ammount;
		//! mint_val() function is serial.
		int bal = 1000, total = 0;
		for(int sid = 1; sid <= CoinSObj; sid++) 
		{
			//! 5000 is contract deployer.
			bool v = coin->mint(5000, sid, bal);
			total  = total + bal;
		}
		//! giveRightToVote() function is serial.
		for(int voter = 1; voter <= nVoter; voter++) 
		{
			//! 10000 is chairperson.
			ballot->giveRightToVote(10000, voter);
		}

		//!----------------------------------------------------
		//! MASTER THREAD CREATE n VALIDATOR THREADS          !
		//!----------------------------------------------------
		//!Start clock
		double start = Ttimer.timeReq();
		
		thread master = thread(concValidator, 0 );
		master.join();
		
		//!Stop clock
		tTime[1] = Ttimer.timeReq() - start;

		//!print the final state of the shared objects by validator.
//		finalState();
//		string winner;
//		ballot->winnerName(&winner);
//		auction->AuctionEnded( );
	}

	//!-------------------------------------------------------
	//! FUNCTION TO BE EXECUTED BY ALL THE VALIDATOR THREADS.!
	//!-------------------------------------------------------
	static void concValidator( int t_ID )
	{
		Timer Ttimer;
		//! start timer to get time taken by this thread.
		auto start = Ttimer._timeStart();

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
						verTemp->nodeLock.lock();
						if(verTemp->in_count < 0)
						{
							verTemp->nodeLock.unlock();
							status[t_ID] = 0;
						}
						else
						{
							verTemp->in_count = -1;
							verTemp->nodeLock.unlock();

							//! get AU to execute, which is of string type;
							//! listAUs index statrt with 0 => -1.
							istringstream ss(listAUs[(verTemp->AU_ID)-1]);
							string tmp;

							//! AU_ID to Execute.
							ss >> tmp;
							int AU_ID = stoi(tmp);

							//! Function Name (smart contract).
							ss >> tmp;
							if( tmp.compare("get_bal") == 0 )
							{
								//! get balance of SObj/id.
								ss >> tmp;
								int s_id = stoi(tmp);
								int bal  = 0;

								//! get_bal() of smart contract.
								bool v = coin->get_bal(s_id, &bal);
							}
							if( tmp.compare("send") == 0 )
							{
								//! Sender ID.
								ss >> tmp;
								int s_id = stoi(tmp);

								//! Reciver ID.
								ss >> tmp;
								int r_id = stoi(tmp);

								//! Ammount to Send.
								ss >> tmp;
								int amt = stoi(tmp);
								bool v  = coin->send(s_id, r_id, amt);
							}
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

							//!-----------------------------------------
							//!change indegree of out edge nodes (node !
							//! having incomming edge from this node). !
							//!-----------------------------------------
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
		vTTime[t_ID] = vTTime[t_ID] + Ttimer._timeStop( start );
	}

	//!----------------------------------
	//!FINAL STATE OF ALL THE SHARED    |
	//!OBJECT. Once all AUs executed.   |
	//!----------------------------------
	void finalState()
	{
		int total = 0;
		cout<<pl<<"SHARED OBJECTS FINAL STATE\n"<<pl
			<<"ACCT ID | FINAL STATE\n"<<pl;
		for(int sid = 1; sid <= CoinSObj; sid++) 
		{
			int bal = 0;
			//! get_bal() of smart contract.
			bool v = coin->get_bal(sid, &bal);
			total  = total + bal;
			cout<<"  "+to_string(sid)+"  \t|  "+to_string(bal)+"\n";
		}
		cout<<pl<<"  SUM   |  "<<total<<"\n"<<pl;
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
	//! list holds the avg time taken by miner and validator
	//!  threads for multiple consecutive runs of the algorithm.
	list<double>mItrT;         
	list<double>vItrT;	

	FILEOPR file_opr;
	
	//! read from input file::
	int input[7] = {0};
	file_opr.getInp(input);
	nThread   = input[0]; // # of threads; should be one for serial;
	numAUs    = input[1]; // # of AUs or Transactions;
	CoinSObj  = input[2]; // # Coin Account Shared Object.
	nProposal = input[3]; // # Ballot Proposal Shared Object.
	nVoter    = input[4]; // # Ballot Voter Shared Object.
	nBidder   = input[5]; // # Auction Bidder Shared Object.
	bidEndT   = input[6]; // # Auction Bid End Time.

	//! max shared object error handling.
	if(CoinSObj > maxCoinAObj) 
	{
		CoinSObj = maxCoinAObj;
		cout<<"Max number of Coin Shared Object can be "<<maxCoinAObj<<"\n";
	}
	if(nProposal > maxPropObj) 
	{
		nProposal = maxPropObj;
		cout<<"Max number of Proposal Shared Object can be "<<maxPropObj<<"\n";
	}	
	if(nVoter > maxVoterObj) 
	{
		nVoter = maxVoterObj;
		cout<<"Max number of Voter Shared Object can be "<<maxVoterObj<<"\n";
	}
	if(nBidder > maxBidrObj) 
	{
		nBidder = maxBidrObj;
		cout<<"Max number of Bid Shared Object can be "<<maxBidrObj<<"\n";
	}
	if(bidEndT > maxbEndT) 
	{
		bidEndT = maxbEndT;
		cout<<"Max Bid End Time can be "<<maxbEndT<<"\n";
	}


	cout<<pl<<"Mixed MVTO Fine Grain Fork-Join Algorithm \n"<<pl;
	int totalRun = 1;
	for(int numItr = 0; numItr < totalRun; numItr++)
	{
		lib = new MVTO;
 		//! generates AUs (i.e. trans to be executed by miner & validator).
		file_opr.genAUs(input, CFCount, BFCount, AFConut, listAUs);

		mAUT = new std::atomic<int>[numAUs];

		//MINER
		Miner *miner = new Miner(0);
		miner ->mainMiner();
		
		//VALIDATOR
		Validator *validator = new Validator();
		validator ->mainValidator();

	
		
		float_t gConstT = 0;
		for(int ii = 0; ii < nThread; ii++)
		{
			gConstT += gTtime[ii];
		}
		cout<<"Avg Grpah Time= "<<gConstT/nThread<<" microseconds";
		
		
		//! total valid AUs among List-AUs executed
		//! by Miner & varified by Validator.
		int vAUs = numAUs - aCount[0];
		file_opr.writeOpt(nThread, numAUs, tTime, 
							mTTime, vTTime, aCount, vAUs, mItrT, vItrT);
		cout<<"\n===================== Execution "
			<<numItr+1<<" Over =====================\n"<<endl;
		
		listAUs.clear();
		lib = NULL;
		delete miner;
		miner = NULL;
		delete cGraph;
		cGraph = NULL;
		delete validator;
		validator = NULL;
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
return 0;
}
