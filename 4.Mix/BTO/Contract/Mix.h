/********************************************************************
|  *****     File:   Mix.h                                   *****  |
|  *****  Created:   Oct 04, 2018, 10:46 AM                  *****  |
********************************************************************/

#pragma once
#include <chrono>
#include "STMBTO/STM.h"
#include "STMBTO/BTO.cpp"
#define CIDStart 5000  //Coin shared objects ids from 5000-10000.
#define BIDStart 10000 //Ballot contract shared objects ids from 10000 onwords.

using namespace std;

//! using BTO protocol of STM library.
STM* lib = new BTO;

class SimpleAuction
{
	public:

		// Parameters of the auction. Times are either absolute unix time-
		// -stamps (seconds since 1970-01-01) or time periods in seconds.		
		std::chrono::time_point<std::chrono::system_clock> now, start;
		
		// beneficiary of the auction.
		int beneficiary;
		int beneficiaryAmount = 0;
		
		
		std::atomic<int> auctionEnd;

		// Current state of the auction (USED BY VALIDATOR).
		std::atomic<int> highestBidder;
		std::atomic<int> highestBid;

		struct PendReturn
		{
			int ID;
			int value;
		};
		// Allowed withdrawals of previous bids.
		// mapping(address => uint) pendingReturns;
		list<PendReturn>pendingReturns;

		// Set to true at the end, disallows any change (USED BY VALIDATOR).
		std::atomic<bool> ended ;

		// The following is a so-called natspec comment,recognizable by the 3
		// slashes. It will be shown when the user is asked to confirm a trans.
		/// Create a simple auction with \`_biddingTime`\, seconds bidding
		/// time on behalf of the beneficiary address \`_beneficiary`\.
		
		// CONSTRUCTOR.
		SimpleAuction( int _biddingTime, int _beneficiary, int numBidder)
		{
			//! USED BY VALIDATOR ONLY.
			beneficiary    = _beneficiary;
			highestBidder  = 0;
			highestBid     = 0;
			ended          = false;
			 
			long long error_id;
			//! \'ended'\ in STM memory, USED BY MINERS.
			if(lib->create_new(0, sizeof(bool)) != SUCCESS)
				cout<<"\nFailed to create Shared Object 'ended' in STM\n";
			
			//! \'highestBidder'\ in STM memory, USED BY MINERS.
			if(lib->create_new(-1, sizeof(int)) != SUCCESS)
				cout<<"\nFailed to create Shared Object 'highestBidder' in STM\n";

			//! \'highestBid'\ in STM memory, USED BY MINERS.
			if(lib->create_new(-2, sizeof(int)) != SUCCESS)
				cout<<"\nFailed to create Shared Object 'highestBid' in STM\n";

			for(int b = 1; b <= numBidder; b++)
			{
				//! USED BY VALIDATORS.
				PendReturn pret;
				pret.ID = b;
				pret.value = 0;
				pendingReturns.push_back(pret);
				
				//! \'pendingReturns[]'\ memory in STM memory, USED BY MINERS.
				if(lib->create_new(b, sizeof(int) ) != SUCCESS)
					cout<<"\nFailed to create object pendingReturns[] in STM\n";
					
				//! initilize bidder pendingReturns[] value = 0 in STM Memory;
				long long error_id;
				trans_state *T   = lib->begin();  //! Transactions begin.
				common_tOB *bidr = new common_tOB;
				bidr->size       = sizeof(int);
				bidr->ID         = b;            //! Bidder SObj in STM memory. 
				
				int bR = lib->read(T, bidr);     //! read from STM Shared Memoy.
				*(int*)bidr->value = 0;
				int bW = lib->write(T, bidr);
				int tc = lib->try_commit(T, error_id);
			}
						
			start = std::chrono::system_clock::now();
			cout<<"AUCTION [Start Time = "<<0;
			auctionEnd = _biddingTime;
			cout<<"] [End Time = "<<auctionEnd<<"] milliseconds\n";
		};

		
		
		/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
		/*!   FUNCTION FOR VALIDATOR   !*/
		/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
		// Bid on the auction with the value sent together with transaction.
		// The value will only be refunded if the auction is not won.
		bool bid( int payable, int bidderID, int bidValue );
		
		// Withdraw a bid that was overbid.
		bool withdraw(int bidderID);
				
		// End the auction and send the highest bid to the beneficiary.
		bool auction_end();
		void AuctionEnded( );
		int send(int bidderID, int amount);
		void reset();

		/*~~~~~~~~~~~~~~~~~~~~~~*/
		/*! FUNCTION FOR MINER.!*/
		/*~~~~~~~~~~~~~~~~~~~~~~*/
		int bid_m( int payable, int bidderID, int bidValue, 
					int *ts, list<int> &cList);
		int withdraw_m(int bidderID, int *ts, list<int> &cList);
		int auction_end_m(int *ts, list<int> &cList);
		bool AuctionEnded_m( );
		int send_m(int bidderID, int amount);
		
	~SimpleAuction()
	{
	};//destructor
};




class Coin
{
	private:
		struct accNode
		{
			int ID;
			int bal;
		};
		list<accNode>listAccount;
		
		std::atomic<int> minter;         //! contract creator
		
	public:
		Coin(int m, int minter_id)       //! constructor
		{
			long long error_id;
			minter  = minter_id;         //! minter is contract creator
			
			for(int i = CIDStart+1; i <= CIDStart+m; i++)
			{			
				accNode acc;
				acc.ID = i-CIDStart;
				acc.bal = 0;
				
				listAccount.push_back(acc);

				if(lib->create_new(i, sizeof(int) ) != SUCCESS)
					cout<<"\nError!!Failed to create Shared Object"<<endl;
			}
		};
		
		/*!!! STANDERED COIN CONTRACT FUNCTION FROM SOLIDITY CONVERTED IN C++ USED BY validator !!!*/
		bool mint(int t_ID, int receiver_iD, int amount);     //! serial function1 for validator.
		bool send(int sender_iD, int receiver_iD, int amount);//! concurrent function1 for validator.
		bool get_bal(int account_iD, int *bal);               //! concurrent function2 for validator.
		
		
		/*!!! CONTRACT with 3 functions for miner return TRUE/1 if Try_Commit SUCCESS !!!*/

		bool mint_m(int t_ID, int receiver_iD, int amount, int *time_stamp);

		int send_m(int t_ID, int sender_iD, int receiver_iD, 
						int amount, int *time_stamp, list<int>&conf_list);
		
		bool get_bal_m(int account_iD, int *bal, int t_ID,
							int *time_stamp, list<int>&conf_list);
		
		~Coin()
		{
		};//destructor
};






class Ballot
{
public:
		int numProposal;
		struct Voter
		{
			int ID;
			int weight;  // weight is accumulated by delegation
			bool voted;  // if true, that person already voted
			int delegate;// person delegated to
			int vote;    // index of the voted proposal
		};
		
		list<Voter>voters;//! this is voter shared object used by validator.

		//! This is a type for a single proposal.
		struct Proposal
		{
			int ID;
			string *name; //! short name (<=32 bytes) data type only avail in solidity
			int voteCount;//! number of accumulated votes
		};

		list<Proposal>proposals;
		
		std::atomic<int> chairperson;
		
		//! constructor:: create a new ballot to choose one of `proposalNames`.
		Ballot(string proposalNames[], int sender, int numVoter, int nPropsal)
		{
			long long error;
			
			numProposal = nPropsal;
			//! sender is chairperson of the contract
			chairperson = sender;

			// This declares a state variable that
			// stores a \`Voter\` struct for each possible address.
			// mapping(address => Voter) public voters;
			for(int v = BIDStart; v <= BIDStart+numVoter; v++)
			{
				//! create voter shared objects in STM BTO 
				//! library which is used by miner.
				if( lib->create_new(v, sizeof(Voter) ) != SUCCESS)
				{
					cout<<"\nERROR:: Failed to create Shared Voter Object"<<endl;
				}
				
				//! Initializing shared objects in 
				//! STM BTO library which is used by miner.
				trans_state * T   = lib->begin();
				common_tOB * vObj = new common_tOB;
				vObj->size        = sizeof(Voter);
				vObj->ID          = v;
				
				lib->read(T,vObj);
				
				if(v == chairperson)
				{
					//if sender id is chairperson.
					(*(Voter*)vObj->value).weight = 1;
				}
				else
				{
					//! '0' indicates that it doesn't have right to vote.
					(*(Voter *)vObj->value).weight   = 0;
					
					//! 'false' indicates that it hasn't vote yet.
					(*(Voter *)vObj->value).voted    = false;
					
					//! denotes to whom voter is going select to vote 
					//! on behalf of this voter && '0' indicates
					//! that it hasn't delegate yet.
					(*(Voter *)vObj->value).delegate = 0;
					
					//! denotes to whom voter has votes && 
					//! '0' indicates that it hasn't vote yet.
					(*(Voter *)vObj->value).vote     = 0;
				}
				lib->write(T, vObj);
				lib->try_commit(T, error);
				
				//! Initializing shared objects used by validator.
				Voter votr;
				votr.ID       = v-BIDStart;
				votr.voted    = false;
				votr.delegate = 0;
				votr.vote     = 0;
				if(v == chairperson)
				{
					votr.weight = 1; //if senderid is chairperson;
				}
				voters.push_back(votr);
			}
		
			// A dynamically-sized array of \`Proposal\` structs.
			/*! Proposal[] public proposals;*/
			
			// \`Proposal({...})\` creates a temporary
			// Proposal object and \`proposals.push(...)\`
			// appends it to the end of \`proposals\`.
			//! For each of the provided proposal names,
			//! create, initilize and add new proposal 
			//! shared object to STM shared memory.
			
			for(int p = BIDStart+1; p <= BIDStart+numProposal; p++)
			{	
				if( lib->create_new(-p, sizeof(Proposal)) != SUCCESS)
				{
					cout<<"\nERROR:: Failed to create Proposal Shared Object"<<endl;
				}
				/*! proposals.push(Proposal({name: propName[i], voteCount: 0}));*/
				trans_state * T   = lib->begin();
				common_tOB * pObj = new common_tOB;
				pObj->size        = sizeof(Proposal);
				pObj->ID          = -p;
				
				int pR = lib->read(T, pObj);
				
				(*(Proposal *)pObj->value).name = &proposalNames[p-1-BIDStart];
				(*(Proposal *)pObj->value).voteCount = 0;//! Denotes voteCount of candidate.

				int pW = lib->write(T, pObj);
				lib->try_commit(T, error);
//					cout<<"Proposal ID \t= "<<pObj->ID<<endl;
//					cout<<"Name \t\t= " <<*(*(Proposal *)pObj->value).name<<endl;
//					cout<<"Vote Count \t= "<<(*(Proposal *)pObj->value).voteCount;
//					cout<<"\n==========================\n";
				
				//! Initializing Proposals used by validator.
				Proposal prop;
				prop.ID        = p-BIDStart;
				prop.voteCount = 0;
				prop.name      = &proposalNames[p-1-BIDStart];
				proposals.push_back(prop);
			}
		};
		
	//!~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	//! fun called by the validator threads.
	//!~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// Give \`voter\` the right to vote on this ballot.
	// May only be called by \`chairperson\`.
	void giveRightToVote(int senderID, int voter);
	
	// Delegate your vote to the voter \`to\`.
	int delegate(int senderID, int to);
	
	// Give your vote (including votes delegated to you)
	// to proposal \`proposals[proposal].name\`.
	int vote(int senderID, int proposal);
	
	// @dev Computes the winning proposal taking all
	// previous votes into account.
	int winningProposal( );
	
	// Calls winningProposal() function to get the 
	// index of the winner contained in the proposals
    // array and then returns the name of the winner.
	void winnerName(string *winnerName);
	
	void state(int ID, bool sFlag);
	
	//!~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	//! fun called by the miner threads.
	//!~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	int giveRightToVote_m(int senderID, int voter);
	int delegate_m(int senderID, int to, int *ts, list<int>&cList);
	int vote_m( int senderID, int proposal, int *ts, list<int>&cList);
	int winningProposal_m( );
	void winnerName_m(string *winnerName);
	void state_m(int ID, bool sFlag);
	
	~Ballot(){ };//destructor
};
