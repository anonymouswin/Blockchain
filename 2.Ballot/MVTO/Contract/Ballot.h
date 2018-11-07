#pragma once
#include "STMMVTO/STM.h"
#include "STMMVTO/MVTO.cpp"

using namespace std;

class Ballot
{

public:
		STM* lib = new MVTO;  //! using BTO protocol of STM library.
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
			string *name;   //! short name (<=32 bytes) data type only avail in solidity
			int voteCount; //! number of accumulated votes
		};

		list<Proposal>proposals;
		
		std::atomic<int> chairperson;
		
		//! constructor:: create a new ballot to choose one of `proposalNames`.
		Ballot(string proposalNames[], int sender, int numVoter, int nPropsal)
		{
			long long error;
			
			numProposal = nPropsal;
//			cout<<"\n==================================";
//			cout<<"\n     Number of Proposal = "<<numProposal;
//			cout<<"\n==================================\n";

			//! sid is chairperson of the contract
			chairperson = sender;

			// This declares a state variable that
			// stores a \`Voter\` struct for each possible address.
			// mapping(address => Voter) public voters;
			for(int v = 0; v <= numVoter; v++)
			{
				//! create voter shared objects in STM BTO library which is used by miner.
				if( lib->create_new(v, sizeof(Voter) ) != SUCCESS)
				{
					cout<<"\nERROR:: Failed to create Shared Voter Object"<<endl;
				}
				
				//! Initializing shared objects in STM BTO library which is used by miner.
				trans_state * T   = lib->begin();
				common_tOB * vObj = new common_tOB;
				vObj->size        = sizeof(Voter);
				vObj->ID          = v;
				
				lib->read(T,vObj);
				
				if(v == chairperson)
				{
					(*(Voter*)vObj->value).weight = 1; //if senderid is chairperson
				}
				else
				{
					(*(Voter *)vObj->value).weight   = 0;      //! '0' indicates that it doesn't have right to vote
					(*(Voter *)vObj->value).voted    = false;  //! 'false' indicates that it hasn't vote yet
					(*(Voter *)vObj->value).delegate = 0;      //! denotes to whom voter is going select to vote on behalf of this voter && '0' indicates that it hasn't delegate yet
					(*(Voter *)vObj->value).vote     = 0;      //! denotes to whom voter has votes && '0' indicates that it hasn't vote yet.
				}
				lib->write(T, vObj);
				lib->try_commit(T, error);
				
				//! Initializing shared objects used by validator.
				Voter votr;
				votr.ID       = v;
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
			
			for(int p = 1; p <= numProposal; p++)
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
				
				(*(Proposal *)pObj->value).name = &proposalNames[p-1];
				(*(Proposal *)pObj->value).voteCount = 0;	//! Denotes voteCount of candidate.

				int pW = lib->write(T, pObj);
				lib->try_commit(T, error);
//					cout<<"Proposal ID \t= "<<pObj->ID<<endl;
//					cout<<"Name \t\t= " <<*(*(Proposal *)pObj->value).name<<endl;
//					cout<<"Vote Count \t= "<<(*(Proposal *)pObj->value).voteCount;
//					cout<<"\n==========================\n";
				
				//! Initializing Proposals used by validator.
				Proposal prop;
				prop.ID        = p;
				prop.voteCount = 0;
				prop.name      = &proposalNames[p-1];
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
