#include "Ballot.h"

	//!~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	//! fun called by the validator threads.
	//!~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
/*----------------------------------------------------
! Validator::Give \`voter\` the right to vote on     !
! this ballot. May only be called by \`chairperson\`.!
*---------------------------------------------------*/
void Ballot::giveRightToVote(int senderID, int voter)
{
	/*! Ballot::Voter voter[Voter_Count+1];*/

	// If 'sender' is not a chairperson, return,
	// as only chairperson can give rights to vote.
	// \`throw\` terminates & reverts all changes to
	// the state and to Ether balances. It is often
	// a good idea to use this if functions are
	// called incorrectly. But watch out, this
	// will also consume all provided gas.
	list<Voter>::iterator it = voters.begin();
	for(; it != voters.end(); it++)
	{
		if( it->ID == voter) break;
	}
	if(it == voters.end() && it->ID != voter)
	{
		cout<<"\nError:: voter "+to_string(voter)+" not found.\n";
	}

	if(senderID != chairperson || it->voted == true)
	{
		if(senderID != chairperson)
			cout<< "Error: Only chairperson can give right.\n";
		else
			cout<< "Error: Already Voted.\n";
		return;
	}
	it->weight = 1;
}

/*-----------------------------------------------------
! Validator:: Delegate your vote to the voter \`to\`. !
*----------------------------------------------------*/
int Ballot::delegate(int senderID, int to)
{
	// assigns reference.
	list<Voter>::iterator sender = voters.begin();
	for(; sender != voters.end(); sender++)
	{
		if( sender->ID == senderID)
			break;
	}
	if(sender == voters.end() && sender->ID != senderID)
	{
		cout<<"\nError:: voter "+to_string(senderID)+" not found.\n";
	}
        
	if(sender->voted)
	{
		//cout<<"\nVoter "+to_string(senderID)+" already voted!\n";
		return 0;//already voted.
	}
	// Forward the delegation as long as \`to\` also delegated.
	// In general, such loops are very dangerous, because if 
	// they run too long, they might need more gas than is 
	// available in a block. In this case, delegation will 
	// not be executed, but in other situations, such loops 
	// might cause a contract to get "stuck" completely.
	
	list<Voter>::iterator Ito = voters.begin();
	for(; Ito != voters.end(); Ito++)
	{
		if( Ito->ID == to)
			break;
	}
	if(Ito == voters.end() && Ito->ID != to)
	{
		cout<<"\nError:: voter to "+to_string(to)+" not found.\n";
	}

	while ( Ito->delegate != 0 && Ito->delegate != senderID ) 
	{
		to = Ito->delegate;
		for(Ito = voters.begin(); Ito != voters.end(); Ito++)
		{
			if( Ito->ID == to)
				break;
		}
		if(Ito == voters.end() && Ito->ID != to)
		{
			cout<<"\nError:: voter to "+to_string(to)+" not found.\n";
		}	
	}
	// We found a loop in the delegation, not allowed.
	if (to == senderID)
	{
		return -1;
	}
	// Since \`sender\` is a reference, this
	// modifies \`voters[msg.sender].voted\`
	sender->voted    = true;
	sender->delegate = to;
	
	list<Voter>::iterator delegate = voters.begin();
	for(; delegate != voters.end(); delegate++)
	{
		if( delegate->ID == to)
			break;
	}
	if(delegate == voters.end() && delegate->ID != to)
	{
		cout<<"\nError:: voter to "+to_string(to)+" not found.\n";
	}
	if (delegate->voted) 
	{
		list<Proposal>::iterator prop = proposals.begin();
		for(; prop != proposals.end(); prop++)
		{
			if( prop->ID == delegate->vote)
				break;
		}
		if(prop == proposals.end() && prop->ID != delegate->vote)
		{
			cout<<"\nError:: Proposal "+to_string(delegate->vote)+" not found.\n";
		}	
		// If the delegate already voted,
		// directly add to the number of votes
		prop->voteCount = prop->voteCount + sender->weight;
		sender->weight = 0;
		return 1;
	}
	else
	{
		// If the delegate did not voted yet,
		// add to her weight.
		delegate->weight = delegate->weight + sender->weight;
		return 1;
	}
}

/*-------------------------------------------------------
! Validator:: Give your vote (including votes delegated !
! to you) to proposal \`proposals[proposal].name\`     .!
-------------------------------------------------------*/
int Ballot::vote(int senderID, int proposal)
{
	list<Voter>::iterator sender = voters.begin();
	for(; sender != voters.end(); sender++)
	{
		if( sender->ID == senderID)
			break;
	}
	if(sender == voters.end() && sender->ID != senderID)
	{
		cout<<"\nError:: voter "+to_string(senderID)+" not found.\n";
	}

	if (sender->voted) return 0;//already voted

	sender->voted = true;
	sender->vote  = proposal;

	// If \`proposal\` is out of the range of the array,
	// this will throw automatically and revert all changes.
	list<Proposal>::iterator prop = proposals.begin();
	for(; prop != proposals.end(); prop++)
	{
		if( prop->ID == proposal)
			break;
	}
	if(prop == proposals.end() && prop->ID != proposal)
	{
		cout<<"\nError:: Proposal "+to_string(proposal)+" not found.\n";
	}			
	prop->voteCount += sender->weight;
	return 1;
}

/*-------------------------------------------------
! Validator:: @dev Computes the winning proposal  !
! taking all previous votes into account.        .!
-------------------------------------------------*/
int Ballot::winningProposal( )
{
	int winningProposal  = 0;
	int winningVoteCount = 0;
	for (int p = 1; p <= numProposal; p++)//numProposal = proposals.length.
	{
		//Proposal *prop = &proposals[p];
		list<Proposal>::iterator prop = proposals.begin();
		for(; prop != proposals.end(); prop++)
		{
			if( prop->ID == p)
				break;
		}
		if(prop == proposals.end() && prop->ID != p)
		{
			cout<<"\nError:: Proposal "+to_string(p)+" not found.\n";
		}		
		if (prop->voteCount > winningVoteCount)
		{
			winningVoteCount = prop->voteCount;
			winningProposal = p;
		}
	}
	cout<<"\n=======================================================\n";
	cout<<"Winning Proposal = " <<winningProposal<< 
			" | Vote Count = "<<winningVoteCount;
	return winningProposal;
}

/*-----------------------------------------------------
! Validator:: Calls winningProposal() function to get !
! the index of the winner contained in the proposals  !
! array and then returns the name of the winner.      !
-----------------------------------------------------*/
void Ballot::winnerName(string *winnerName)
{
	int winnerID = winningProposal();
	list<Proposal>::iterator prop = proposals.begin();
	for(; prop != proposals.end(); prop++)
	{
		if( prop->ID == winnerID)
			break;
	}
	if(prop == proposals.end() && prop->ID != winnerID)
	{
		cout<<"\nError:: Proposal "+to_string(winnerID)+" not found.\n";
	}

	//winnerName = &(prop)->name;
	cout<<" | Name = " <<*(prop->name) << " |";
	cout<<"\n=======================================================\n";
}

void Ballot::state(int ID, bool sFlag)
{
	cout<<"==========================\n";
	if(sFlag == false)//Proposal
	{
		list<Proposal>::iterator prop = proposals.begin();
		for(; prop != proposals.end(); prop++)
		{
			if( prop->ID == ID)
				break;
		}
		if(prop == proposals.end() && prop->ID != ID)
		{
			cout<<"\nError:: Proposal "+to_string(ID)+" not found.\n";
		}
		cout<<"Proposal ID \t= -"  << prop->ID  <<endl;
		cout<<"Proposal Name \t= "<< *(prop->name) <<endl;
		cout<<"Vote Count \t= "  << prop->voteCount <<endl;
//		cout<<"================================\n";
	}
	if(sFlag == true)
	{
		list<Voter>::iterator it = voters.begin();
		for(; it != voters.end(); it++)
		{
			if( it->ID == ID)
				break;
		}
		if(it == voters.end() && it->ID != ID)
		{
			cout<<"\nError:: voter "+to_string(ID)+" not found.\n";
		}	
		cout<<"Voter ID \t= "  <<it->ID<<endl;
		cout<<"weight \t\t= "  <<it->weight<<endl;
		cout<<"Voted \t\t= "   <<it->voted<<endl;
		cout<<"Delegate \t= "  <<it->delegate<<endl;
		cout<<"Vote Index -\t= "<<it->vote<<endl;
//		cout<<"================================\n";
	}
}


		//!~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		//! fun called by the miner threads.
		//!~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
/*----------------------------------------------------
! Miner::Give \`voter\` the right to vote on         !
! this ballot. May only be called by \`chairperson\`.!
*---------------------------------------------------*/
int Ballot::giveRightToVote_m(int senderID, int voter)
{
	long long error_id; 
	trans_state* T   = lib->begin();
	common_tOB* cObj = new common_tOB;
	cObj->size       = sizeof(Voter);
	cObj->ID         = voter;

	int cR = lib->read(T, cObj);
	if( cR != SUCCESS)
		return 0;// AU transaction aborted.
	
	if(senderID != chairperson || (*(Voter*)cObj->value).voted)
	{
		cout<<"\nVoter "+to_string(senderID)+" already voted or your not chairperson!\n";

		lib->try_abort(T);
		return -1; //invalid AU.
	}
	(*(Voter*)cObj->value).weight = 1;
	
	int cW = lib->write(T, cObj);
//	if(cW != SUCCESS )
	{
//		lib->try_abort( T );
//		return 0;//AU aborted.
	}
	int tc = lib->try_commit(T, error_id);
	if(tc == SUCCESS)	
		return 1;//valid AU: executed successfully.
	else
		return 0; //AU transaction aborted.
}


/*-------------------------------------------------
! Miner:: Delegate your vote to the voter \`to\`. !
*------------------------------------------------*/
int Ballot::delegate_m(int senderID, int to, int *ts, list<int>&cList)
{
	long long error_id;

	trans_state* T    = lib->begin();
	*ts               = T->TID;
	common_tOB* sObj  = new common_tOB;
	sObj->size        = sizeof(Voter);
	sObj->ID          = senderID;

	common_tOB* toObj = new common_tOB;
	toObj->size       = sizeof(Voter);
	toObj->ID         = to;

	int sR = lib->read(T, sObj);
	int tR = lib->read(T, toObj);

	if(sR != SUCCESS && tR != SUCCESS)
		return 0;//AU aborted.

	if( (*(Voter*)sObj->value).voted )
	{
		lib->try_abort(T);
		return -1;//AU is invalid
	}
	
	int delegateTO = (*(Voter*)toObj->value).delegate;
	while( delegateTO != 0 )
	{
		if(delegateTO == sObj->ID)
		{
			break;
		}
		
		toObj->ID = (*(Voter*)toObj->value).delegate;
		int r = lib->read(T, toObj);	
		if(r != SUCCESS) return 0;//AU aborted.
		
		delegateTO = (*(Voter*)toObj->value).delegate;
	}

	//! We found a loop in the delegation, not allowed. 
	//! Prev while may cause loop.
	if (toObj->ID == sObj->ID) 
	{
		lib->try_abort(T);
		return -1;//AU is invalid
	}

	(*(Voter*)sObj->value).voted    = true;
	(*(Voter*)sObj->value).delegate = toObj->ID;

	// If the delegate already voted, directly add to the number of votes.
	if( (*(Voter*)toObj->value).voted ) 
	{
		int votedPropsal = (*(Voter*)toObj->value).vote;
		common_tOB* pObj = new common_tOB;
		pObj->size       = sizeof(Proposal);
		pObj->ID         = -votedPropsal;
		
		int tR  = lib->read(T, pObj);
		if( tR != SUCCESS )
			return 0;//AU aborted.
				
		(*(Proposal*)pObj->value).voteCount += (*(Voter*)sObj->value).weight;
				
		(*(Voter*)sObj->value).weight = 0;
		
		int pW = lib->write(T, pObj);
		if(pW != SUCCESS )
		{
			lib->try_abort( T );
			return 0;//AU aborted.
		}
	}
	else // If the delegate did not vote yet, add to weight.
	{
		(*(Voter*)toObj->value).weight += (*(Voter*)sObj->value).weight;
	}
	
	int sW = lib->write(T, sObj);
	int tW = lib->write(T, toObj);
//	if(sW != SUCCESS && tW != SUCCESS )
	{
//		lib->try_abort( T );
//		return 0;//AU aborted.
	}
	
	int tc = lib->try_commit_conflict(T, error_id, cList); 
	if( tc == SUCCESS )
		return 1;//valid AU: executed successfully 
	
	else
		return 0; //AU transaction aborted
}


/*---------------------------------------------------
! Miner:: Give your vote (including votes delegated !
! to you) to proposal \`proposals[proposal].name\  .!
---------------------------------------------------*/
int Ballot::vote_m( int senderID, int proposal, int *ts, list<int>&cList)
{
	long long error;
	trans_state* T   = lib->begin();
	*ts              =  T->TID;
	common_tOB* vObj = new common_tOB;
	vObj->size       = sizeof(Voter);
	vObj->ID         = senderID;
	
	common_tOB* pObj = new common_tOB;
	pObj->size       = sizeof(Proposal);
	pObj->ID         = -proposal;
	
	int rS = lib->read(T, vObj);
	
	if(rS != SUCCESS )
		return 0;// AU aborted.
	
	if( (*(Voter*)vObj->value).voted )
	{
		lib->try_abort(T);
		return -1;// AU is invalid.
	}


	int rP = lib->read(T, pObj);
	if(rP != SUCCESS) return 0;// AU aborted.
	
	(*(Voter*)vObj->value).voted = true;
	(*(Voter*)vObj->value).vote  = -pObj->ID;
	(*(Proposal*)pObj->value).voteCount += (*(Voter*)vObj->value).weight;
	
//	cout<<" new vote count is "+to_string((*(Proposal*)pObj->value).voteCount);
	
	int vW = lib->write(T, vObj);
	int pW = lib->write(T, pObj);
//	if(vW != SUCCESS && pW != SUCCESS )
	{
//		lib->try_abort( T );
//		return 0;//AU aborted.
	}

	int tc = lib->try_commit_conflict(T, error, cList);
	if(tc == SUCCESS)
		return 1;// valid AU: executed successfully.
	else
		return 0;// AU transaction aborted.
}


/*---------------------------------------------
! Miner:: @dev Computes the winning proposal  !
! taking all previous votes into account.    .!
---------------------------------------------*/
int Ballot::winningProposal_m()
{
	long long error;
	trans_state * T  = lib->begin();
	common_tOB *pObj = new common_tOB;
	pObj->size       = sizeof(Proposal);

	int winningProposal  = 0;
	int winningVoteCount = 0;
	for(int p = 1 ; p <= numProposal ; p++)//numProposal = proposals.length.
	{
		pObj->ID = -p;
		int pR   = lib->read(T, pObj);
		if(pR != SUCCESS) cout<<"\nError in reading Proposal "+to_string(pObj->ID)+"  State.\n";

		if(winningVoteCount < (*(Proposal*)pObj->value).voteCount)
		{
			winningProposal  = p;
			winningVoteCount = (*(Proposal*)pObj->value).voteCount;
		}
	}
	lib->try_commit(T, error);
	
	cout<<"\n=======================================================\n";
	cout<<"Winning Proposal = " <<winningProposal<< " | Vote Count = "<<winningVoteCount;
	return winningProposal;
}



/*-----------------------------------------------------
! Miner:: Calls winningProposal() function to get     !
! the index of the winner contained in the proposals  !
! array and then returns the name of the winner.      !
-----------------------------------------------------*/
void Ballot::winnerName_m(string *winnerName)
{
	long long error_id;
	trans_state* T   = lib->begin();
	common_tOB *pObj = new common_tOB;
	pObj->size       = sizeof(Proposal);
	pObj->ID         = -winningProposal_m();
	
	int wR           = lib->read(T, pObj);
	winnerName      = (*(Proposal*)pObj->value).name;	
	
	cout<<" | Name = " <<*winnerName << " |";
	cout<<"\n=======================================================\n";
	lib->try_commit(T, error_id);
}

void Ballot::state_m(int ID, bool sFlag)
{
	long long error_id;
	trans_state* T  = lib->begin();
	common_tOB* Obj = new common_tOB;
	cout<<"==========================\n";
	if(sFlag == false)//Proposal
	{
		Obj->size = sizeof(Proposal);
		Obj->ID   = -ID;
		if(lib->read(T, Obj) != SUCCESS) return;
		cout<<"Proposal ID \t= "  << Obj->ID  <<endl;
		cout<<"Proposal Name \t= "<< *(*(Proposal*)Obj->value).name <<endl;
		cout<<"Vote Count \t= "  << (*(Proposal*)Obj->value).voteCount <<endl;
//		cout<<"================================\n";
	}
	if(sFlag == true)
	{
		Obj->size = sizeof(Voter);
		Obj->ID   = ID;
		if(lib->read(T, Obj) != SUCCESS) return;
		cout<<"Voter ID : "  <<ID<<endl;
		cout<<"weight : "    <<(*(Voter*)Obj->value).weight <<endl;
		cout<<"Voted : "     <<(*(Voter*)Obj->value).voted <<endl;
		cout<<"Delegate : "  <<(*(Voter*)Obj->value).delegate <<endl;
		cout<<"Vote Index : "<<(*(Voter*)Obj->value).vote <<endl;
//		cout<<"================================\n";
	}
	lib->try_commit(T, error_id);
}
