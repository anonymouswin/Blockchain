#include "Mix.h"

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/*!!!FUNCTIONS FOR VALIDATOR !!!*/
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
//! RESETING TIMEPOIN FOR VALIDATOR.
void SimpleAuction::reset()
{
	beneficiaryAmount = 0;
	start = std::chrono::system_clock::now();
	cout<<"AUCTION [Start Time = "<<0;
	cout<<"] [End Time = "<<auctionEnd<<"] milliseconds\n";
}


/*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
/*! VALIDATOR:: Bid on the auction with the value sent together with this  !*/
/*! transaction. The value will only be refunded if the auction is not won.!*/
/*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
bool SimpleAuction::bid( int payable, int bidderID, int bidValue )
{
	// No arguments are necessary, all information is already part of trans
	// -action. The keyword payable is required for the function to be able
	// to receive Ether. Revert the call if the bidding period is over.
	
	auto end = high_resolution_clock::now();
	double_t now = duration_cast<milliseconds>( end - start ).count();

	if( now > auctionEnd)
	{
//		cout<<"\nAuction already ended.";
		return false;
	}
	// If the bid is not higher, send the
	// money back.
	if( bidValue <= highestBid)
	{
//		cout<<"\nThere already is a higher bid.";
		return false;
	}
	if (highestBid != 0) 
	{
		// Sending back the money by simply using highestBidder.send(highestBid)
		// is a security risk because it could execute an untrusted contract.
		// It is always safer to let recipients withdraw their money themselves.
		//pendingReturns[highestBidder] += highestBid;
		
		list<PendReturn>::iterator pr = pendingReturns.begin();
		for(; pr != pendingReturns.end(); pr++)
		{
			if( pr->ID == highestBidder)
				break;
		}
		if(pr == pendingReturns.end() && pr->ID != highestBidder)
		{
			cout<<"\nError:: Bidder "+to_string(highestBidder)+" not found.\n";
		}
		pr->value = highestBid;
	}
	//HighestBidIncreased(bidderID, bidValue);
	highestBidder = bidderID;
	highestBid    = bidValue;

	return true;
}

/*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
/*! VALIDATOR:: Withdraw a bid that was overbid. !*/
/*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
bool SimpleAuction::withdraw(int bidderID)
{
	list<PendReturn>::iterator pr = pendingReturns.begin();
	for(; pr != pendingReturns.end(); pr++)
	{
		if( pr->ID == bidderID) break;
	}
	if(pr == pendingReturns.end() && pr->ID != bidderID)
	{
		cout<<"\nError:: Bidder "+to_string(bidderID)+" not found.\n";
	}

///	int amount = pendingReturns[bidderID];
	int amount = pr->value;
	if (amount > 0) 
	{
		// It is important to set this to zero because the recipient
		// can call this function again as part of the receiving call
		// before `send` returns.
///		pendingReturns[bidderID] = 0;
		pr->value = 0;
		if ( !send(bidderID, amount) )
		{
			// No need to call throw here, just reset the amount owing.
///			pendingReturns[bidderID] = amount;
			pr->value = amount;
			return false;
		}
	}
	return true;
}

/*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
/* VALIDATOR:: this fun can also be impelemted !*/
/* as method call to other smart contract. we  !*/
/* assume this fun always successful in send.  !*/
/*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
int SimpleAuction::send(int bidderID, int amount)
{
//	bidderAcount[bidderID] += amount;
	return 1;
}

/*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
/* VALIDATOR:: End the auction and send the highest bid to the beneficiary. !*/
/*!_________________________________________________________________________!*/
/*! It's good guideline to structure fun that interact with other contracts !*/
/*! (i.e. they call functions or send Ether) into three phases: 1.checking  !*/
/*! conditions, 2.performing actions (potentially changing conditions), 3.  !*/
/*! interacting with other contracts. If these phases mixed up, other cont- !*/
/*! -ract could call back into current contract & modify state or cause     !*/
/*! effects (ether payout) to be performed multiple times. If fun called    !*/
/*! internally include interaction with external contracts, they also have  !*/ 
/*! to be considered interaction with external contracts.                   !*/
/*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
bool SimpleAuction::auction_end()
{
	// 1. Conditions
	auto end     = high_resolution_clock::now();
	double_t now = duration_cast<milliseconds>( end - start ).count();

	if(now < auctionEnd)
    {
//    	cout<< "\nAuction not yet ended.";
    	return false;
    }
	if(!ended)
    {
//    	AuctionEnded(highestBidder, highestBid);
//		cout<<"\nAuctionEnd has already been called.";
    	return true;
	}
	// 2. Effects
	ended = true;
//	AuctionEnded( );

	// 3. Interaction
	///beneficiary.transfer(highestBid);
	beneficiaryAmount = highestBid;
	return true;
}


void SimpleAuction::AuctionEnded( )
{
	cout<<"\n======================================";
	cout<<"\n| Auction Winer ID "+to_string(highestBidder)
			+" |  Amount "+to_string(highestBid);
	cout<<"\n======================================\n";	
}


/*~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/*!! FUNCTIONS FOR MINER !!!*/
/*~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
/*! MINER:: Bid on the auction with the value sent together with this      !*/
/*! transaction. The value will only be refunded if the auction is not won.!*/
/*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
int SimpleAuction::bid_m( int payable, int bidderID, int bidValue, 
							int *ts, list<int> &cList)
{
	auto end     = high_resolution_clock::now();
	double_t now = duration_cast<milliseconds>( end - start ).count();

	if( now > auctionEnd) return -1;//! invalid AUs: Auction already ended.
	
	long long error_id;
	trans_state *T        = lib->begin(); //! Transactions begin.
	*ts                   = T->TID; //! return time_stamp to user.
	common_tOB *higestBid = new common_tOB;
	higestBid->size       = sizeof(int);
	higestBid->ID         = -2;  //! highestBid SObj id is -2. 
	int hbR               = lib->read(T, higestBid); //! read from STM Shared Memoy.
	if(hbR != SUCCESS) return 0; //! AU aborted.
	common_tOB *hBidder  = new common_tOB;
	hBidder->size        = sizeof(int);
	hBidder->ID          = -1; //! highestBidder SObj id is -1. 
	int hbrR             = lib->read(T, hBidder); //! read from STM Shared Memoy.
	if(hbrR != SUCCESS) return 0; //! AU aborted.

	if( bidValue <= *(int*)higestBid->value )
	{
		lib->try_abort( T );
		return -1;//! invalid AUs: There already is a higher bid.
	}	
		
	// If the bid is no longer higher, send the money back to old bidder.
	if (*(int*)higestBid->value != 0) 
	{
		int hBiderID     = *(int*)hBidder->value; //! higestBidder in STM memory. 
		common_tOB *bidr = new common_tOB;
		bidr->size       = sizeof(int);
		bidr->ID         = hBiderID; 
		int bR           = lib->read(T, bidr); //! read from STM Shared Memoy. 
		if(bR != SUCCESS) return 0; //! AU aborted. 
		//pendingReturns[highestBidder] += highestBid;
		*(int*)bidr->value += *(int*)higestBid->value;
		int bW = lib->write(T, bidr);
	}
	// increase the highest bid.
	*(int*)hBidder->value   = bidderID;
	*(int*)higestBid->value = bidValue;

	int hbrW = lib->write(T, hBidder);
	int hbW  = lib->write(T, higestBid);
	int tc   = lib->try_commit_conflict(T, error_id, cList);
	if(tc == SUCCESS) return 1;//bid successfully done; AU execution successful.
	else return 0;//AU aborted.
}

/*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
/*! MINER:: Withdraw a bid that was overbid. !*/
/*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
int SimpleAuction::withdraw_m(int bidderID, int *ts, list<int> &cList)
{
	long long error_id;
	trans_state *T   = lib->begin();     //! Transactions begin.
	*ts              = T->TID;           //! return time_stamp to user.
	common_tOB *bidr = new common_tOB;
	bidr->size       = sizeof(int);
	bidr->ID         = bidderID;        //! Bidder SObj in STM memory. 
	int bR           = lib->read(T, bidr);        //! read from STM Shared Memoy. 
	if(bR != SUCCESS) return 0;         //! AU aborted.
	
	//int amount = pendingReturns[bidderID];
	int amount = *(int*)bidr->value;
	if (amount > 0) 
	{
		//pendingReturns[bidderID] = 0;
		*(int*)bidr->value = 0;
		int bW = lib->write(T, bidr);

		if ( !send(bidderID, amount) )
		{
			// No need to call throw here, just reset the amount owing.
			*(int*)bidr->value = amount;
			int bW = lib->write(T, bidr);
			int tc = lib->try_commit_conflict(T, error_id, cList);
			if( tc == SUCCESS)
			{
				return -1;//AU invalid.
			}
			else
			{
				return 0; //AU aborted.
			}
		}
	}
	int tc = lib->try_commit_conflict(T, error_id, cList);
	if( tc == SUCCESS)
	{
		return 1;//withdraw successfully done; AU execution successful.
	}
	else
	{ 
		return 0; //AU aborted.
	}
}


/*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
/* MINER:: End the auction and send the highest bid to the beneficiary. !*/
/*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
int SimpleAuction::auction_end_m(int *ts, list<int> &cList)
{
	auto end     = high_resolution_clock::now();
	double_t now = duration_cast<milliseconds>( end - start ).count();

	//! Auction ended?
	if(now > auctionEnd) return -1;
    long long error_id;
    trans_state *T   = lib->begin();      //! Transactions begin.
	*ts              = T->TID;            //! return time_stamp to user.
	common_tOB *bEnd = new common_tOB;
	bEnd->size       = sizeof(bool);
	bEnd->ID         = 0;                 //! end SObj id is 0 in STM memory. 
	int bR           = lib->read(T, bEnd);//! read from STM Shared Memoy. 
	if(bR != SUCCESS) return 0;           //! AU aborted.
	bool ended       = *(bool*)bEnd->value;
	if( !ended )
    {
		//! AuctionEnd has already been called.
		lib->try_abort( T );
    	return 1;
	}
//	ended = true;
	*(bool*)bEnd->value = true;
	int bW = lib->write(T, bEnd);
	if( bW != SUCCESS )
	{
		lib->try_abort( T );
		return 0;//AU aborted.
	}
	common_tOB *hBidder = new common_tOB;
	hBidder->size       = sizeof(int);
	hBidder->ID         = -1;                   //! highestBidder SObj id is -1. 
	int hbrR            = lib->read(T, hBidder);//! read from STM Shared Memoy. 
	if(hbrR != SUCCESS) return 0;               //! AU aborted.
	beneficiaryAmount   = *(int*)hBidder->value;
	int tc              = lib->try_commit_conflict(T, error_id, cList);
	if( tc == SUCCESS) return 1;//auction_end successfully done; AU execution successful.
	else return 0; //AU aborted.
}

bool SimpleAuction::AuctionEnded_m( )
{
	long long error_id;
	trans_state *T        = lib->begin();     //! Transactions begin.
	common_tOB *higestBid = new common_tOB;
	higestBid->size       = sizeof(int);
	higestBid->ID         = -2;                     //! highestBid SObj id is -2. 
	int hbR               = lib->read(T, higestBid);//! read from STM Shared Memoy.
	if(hbR != SUCCESS) return false;                //! AU aborted.
	common_tOB *hBidder  = new common_tOB;
	hBidder->size        = sizeof(int);
	hBidder->ID          = -1;                   //! highestBidder SObj id is -1.	
	int hbrR             = lib->read(T, hBidder);//! read from STM Shared Memoy. 
	if(hbrR != SUCCESS) return false;           //! AU aborted.
	cout<<"\n======================================";
	cout<<"\n| Auction Winer ID "+to_string(*(int*)hBidder->value)
			+" |  Amount "+to_string(*(int*)higestBid->value);
	cout<<"\n======================================\n";	
	
	int tc = lib->try_commit(T, error_id);
	if( tc == SUCCESS) return true;//auction_end successfully done; AU execution successful.
	else return false; //AU aborted.
}



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
		if(senderID != chairperson) cout<< "Error: Only chairperson can give right.\n";
		else cout<< "Error: Already Voted.\n";
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
		if( sender->ID == senderID) break;
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
		if( Ito->ID == to) break;
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
			if( Ito->ID == to) break;
		}
		if(Ito == voters.end() && Ito->ID != to)
		{
			cout<<"\nError:: voter to "+to_string(to)+" not found.\n";
		}	
	}
	// We found a loop in the delegation, not allowed.
	if (to == senderID) return -1;

	// Since \`sender\` is a reference, this
	// modifies \`voters[msg.sender].voted\`
	sender->voted    = true;
	sender->delegate = to;
	
	list<Voter>::iterator delegate = voters.begin();
	for(; delegate != voters.end(); delegate++)
	{
		if(delegate->ID == to) break;
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
			if( prop->ID == delegate->vote) break;
		}
		if(prop == proposals.end() && prop->ID != delegate->vote)
		{
			cout<<"\nError:: Proposal "+to_string(delegate->vote)+
					" not found.\n";
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
		if( sender->ID == senderID) break;
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
		if( prop->ID == proposal) break;
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
			if( prop->ID == p) break;
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
		if( prop->ID == winnerID) break;
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
			if( prop->ID == ID) break;
		}
		if(prop == proposals.end() && prop->ID != ID)
		{
			cout<<"\nError:: Proposal "+to_string(ID)+" not found.\n";
		}
		cout<<"Proposal ID \t= "  << prop->ID <<endl;
		cout<<"Proposal Name \t= "<< *(prop->name) <<endl;
		cout<<"Vote Count \t= "  << prop->voteCount <<endl;
//		cout<<"================================\n";
	}
	if(sFlag == true)
	{
		list<Voter>::iterator it = voters.begin();
		for(; it != voters.end(); it++)
		{
			if( it->ID == ID) break;
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
	cObj->ID         = BIDStart+voter;
	int cR           = lib->read(T, cObj);
	if( cR != SUCCESS) return 0;// AU transaction aborted.
	if(senderID != chairperson || (*(Voter*)cObj->value).voted)
	{
		cout<<"\nVoter "+to_string(senderID)+
				" already voted or your not chairperson!\n";
		lib->try_abort(T);
		return -1; //invalid AU.
	}
	(*(Voter*)cObj->value).weight = 1;
	int cW = lib->write(T, cObj);
	int tc = lib->try_commit(T, error_id);
	if(tc == SUCCESS) return 1;//valid AU: executed successfully.
	else return 0; //AU transaction aborted.
}


/*-------------------------------------------------
! Miner:: Delegate your vote to the voter \`to\`. !
*------------------------------------------------*/
int Ballot::delegate_m(int senderID, int to, int *ts, list<int>&cList)
{
	long long error_id;
	trans_state* T   = lib->begin();
	*ts              = T->TID;
	common_tOB* sObj = new common_tOB;
	sObj->size       = sizeof(Voter);
	sObj->ID         = BIDStart+senderID;
	int sR           = lib->read(T, sObj);
	if(sR != SUCCESS) return 0;//AU aborted.
	if( (*(Voter*)sObj->value).voted )
	{
		lib->try_abort(T);
		return -1;//AU is invalid
	}
	common_tOB* toObj = new common_tOB;
	toObj->size       = sizeof(Voter);
	toObj->ID         = BIDStart+to;
	int tR            = lib->read(T, toObj);
	if(tR != SUCCESS) return 0;//AU aborted.
	int delegateTO    = (*(Voter*)toObj->value).delegate;
	while( delegateTO != 0 )
	{
		if(delegateTO == sObj->ID) break;
		toObj->ID     = (*(Voter*)toObj->value).delegate;
		int r         = lib->read(T, toObj);	
		if(r != SUCCESS) return 0;//AU aborted.
		delegateTO    = (*(Voter*)toObj->value).delegate;
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
		pObj->ID         = votedPropsal;
		int tR           = lib->read(T, pObj);
		if( tR != SUCCESS ) return 0;//AU aborted.
				
		(*(Proposal*)pObj->value).voteCount += (*(Voter*)sObj->value).weight;
		(*(Voter*)sObj->value).weight = 0;
		int pW = lib->write(T, pObj);
	}
	else // If the delegate did not vote yet, add to weight.
	{
		(*(Voter*)toObj->value).weight += (*(Voter*)sObj->value).weight;
	}
	
	int sW = lib->write(T, sObj);
	int tW = lib->write(T, toObj);

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
	vObj->ID         = BIDStart+senderID;
	
	common_tOB* pObj = new common_tOB;
	pObj->size       = sizeof(Proposal);
	pObj->ID         = -(BIDStart+proposal);
	
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
	(*(Voter*)vObj->value).vote  = pObj->ID;
	(*(Proposal*)pObj->value).voteCount += (*(Voter*)vObj->value).weight;
	
//	cout<<" new vote count is "+to_string((*(Proposal*)pObj->value).voteCount);
	
	int vW = lib->write(T, vObj);
	int pW = lib->write(T, pObj);

	int tc = lib->try_commit_conflict(T, error, cList);
	if(tc == SUCCESS) return 1;// valid AU: executed successfully.
	else return 0;// AU transaction aborted.
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
	for(int p = BIDStart+1 ; p <= BIDStart+numProposal ; p++)
	{
		pObj->ID = -p;
		int pR   = lib->read(T, pObj);
		if(pR != SUCCESS) 
			cout<<"\nError in reading Proposal "+to_string(pObj->ID)+"  State.\n";

		if(winningVoteCount < (*(Proposal*)pObj->value).voteCount)
		{
			winningProposal  = p;
			winningVoteCount = (*(Proposal*)pObj->value).voteCount;
		}
	}
	lib->try_commit(T, error);
	
	cout<<"\n=======================================================\n";
	cout<<"Winning Proposal = " <<winningProposal-BIDStart
		<< " | Vote Count = "<<winningVoteCount;
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
	winnerName       = (*(Proposal*)pObj->value).name;	
	
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
		Obj->ID   = -(BIDStart+ID);
		if(lib->read(T, Obj) != SUCCESS) return;
		cout<<"Proposal ID \t= "  << ID  <<endl;
		cout<<"Proposal Name \t= "<< *(*(Proposal*)Obj->value).name <<endl;
		cout<<"Vote Count \t= "  << (*(Proposal*)Obj->value).voteCount <<endl;
//		cout<<"================================\n";
	}
	if(sFlag == true)
	{
		Obj->size = sizeof(Voter);
		Obj->ID   = BIDStart+ID;
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




/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/*!!!functions for validator !!!*/
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
/*!!!            mint() called by minter thread.               !!!*/
/*!!!    initially credit some num of coins to each account    !!!*/
/*!!! (can be called by anyone but only minter can do changes) !!!*/
/*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
bool Coin::mint(int t_ID, int receiver_iD, int amount)
{
	if(t_ID != minter) 
	{
		cout<<"\nERROR:: Only ''MINTER'' (Contract Creator) can initialize the Accounts (Shared Objects)\n";
		return false; //false not a minter.
	}
	list<accNode>::iterator it = listAccount.begin();
	for(; it != listAccount.end(); it++)
	{
		if( (it)->ID == receiver_iD)
			(it)->bal = amount;
	}
	
//	account[receiver_iD] = account[receiver_iD] + amount;
	return true;      //AU execution successful.
}

/*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
/*!!!       send() called by account holder thread.            !!!*/
/*!!!   To send some coin from his account to another account  !!!*/
/*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
bool Coin::send(int sender_iD, int receiver_iD, int amount)
{
	list<accNode>::iterator sender = listAccount.begin();
	for(; sender != listAccount.end(); sender++)
	{
		if( sender->ID == sender_iD)
			break;
	}

//	if(amount > account[sender_iD]) 
//		return false; //not sufficent balance to send; AU is invalid.
	if( sender != listAccount.end() )
	{
		if(amount > sender->bal)
			return false; //not sufficent balance to send; AU is invalid.
	}
	else
	{
		cout<<"Sender ID" << sender_iD<<" not found\n";
		return false;//AU execution fail;
	}
	
	list<accNode>::iterator reciver = listAccount.begin();
	for(; reciver != listAccount.end(); reciver++)
	{
		if( reciver->ID == receiver_iD)
			break;
	}
//	account[sender_iD]   = account[sender_iD]   - amount;
//	account[receiver_iD] = account[receiver_iD] + amount;
	if( reciver != listAccount.end() )
	{
		sender->bal  -= amount;
		reciver->bal += amount;
		return true; //AU execution successful.
	}
	else
	{
		cout<<"Reciver ID" << receiver_iD<<" not found\n";
		return false;// when id not found
	}
}

/*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
/*!!!    get_balance() called by account holder thread.        !!!*/
/*!!!       To view number of coins in his account             !!!*/
/*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
bool Coin::get_bal(int account_iD, int *bal)
{	
	list<accNode>::iterator acc = listAccount.begin();
	for(; acc != listAccount.end(); acc++)
	{
		if( acc->ID == account_iD)
			break;
	}
//	*bal = account[account_iD];
	if( acc != listAccount.end() )
	{
		*bal = acc->bal;
		return true; //AU execution successful.
	}
	else
	{
		cout<<"Account ID" << account_iD<<" not found\n";
		return false;//AU execution fail.
	}
}



/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/*!!!  functions for miner   !!!*/
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
bool Coin::mint_m(int t_ID, int receiver_iD, int amount, int * time_stamp) 
{
	if(t_ID != minter)
	{
		cout<<"\nERROR:: Only ''MINTER'' can initialize the Accounts\n";
		return false;//false not a minter.
	}
	
	long long error_id;
	trans_state* T      = lib->begin();//Transactions begin.
	*time_stamp         = T->TID;      //return time_stamp to user.
	common_tOB* rec_obj = new common_tOB;
	rec_obj->size       = sizeof(int);
	rec_obj->ID         = CIDStart+receiver_iD;
	int r               = lib->read(T, rec_obj);
	if(r != SUCCESS) return false;//AU aborted.

	*(int*)rec_obj->value += amount;
	r = lib->write(T, rec_obj);

	int tryC = lib->try_commit(T, error_id);
	if(tryC == SUCCESS) return true;//AU execution successful.
	else return false;//AU aborted.
}


int Coin::send_m(int t_ID, int sender_iD, int receiver_iD,
						int amount, int *time_stamp, list<int>&conf_list) 
{
	long long error_id;	
	trans_state* T         = lib->begin();//Transactions begin.
	*time_stamp            = T->TID;      //return time_stamp to user.
	common_tOB* sender_obj = new common_tOB;
	sender_obj->size       = sizeof(int);
	sender_obj->ID         = CIDStart+sender_iD;
	int r_s                = lib->read(T, sender_obj );
	if(r_s != SUCCESS)
		return 0;//AU aborted.
	
	common_tOB* rec_obj = new common_tOB;
	rec_obj->size       = sizeof(int);
	rec_obj->ID         = CIDStart+receiver_iD;
	int r_r             = lib->read(T, rec_obj);	
	if(r_r != SUCCESS)
		return 0;//AU aborted.
				
	else if(amount > *(int*)sender_obj->value )
	{
		lib->try_abort( T );
		return -1;//not sufficent balance to send; AU is invalid, Trans aborted.
	}
	
	*(int*)sender_obj->value -= amount;
	*(int*)rec_obj->value    += amount;
		
	r_s = lib->write(T, sender_obj);
	r_r = lib->write(T, rec_obj);
	
	int tryC = lib->try_commit_conflict(T, error_id, conf_list);
	if(tryC == SUCCESS ) return 1;//send done; AU execution successful.
	else return 0;//AU aborted.
}
	

bool Coin::get_bal_m(int account_iD, int *bal, int t_ID,
						int *time_stamp, list<int>&conf_list) 
{
	long long error_id;
	trans_state* T      = lib->begin(); //Transactions begin.
	*time_stamp         = T->TID;       //return time_stamp to user.
	common_tOB* acc_bal = new common_tOB;
	acc_bal->size       = sizeof(int);
	acc_bal->ID         = CIDStart+account_iD;
	int r_a             = lib->read(T, acc_bal);

	if(r_a != SUCCESS) 
		return false;//AU aborted.
	
	*bal = *(int*)acc_bal->value;
	r_a  = lib->try_commit_conflict(T, error_id, conf_list);
	if(r_a == SUCCESS) 
		return true;//AU execution successful.
	else 
		return false;//AU aborted.
}
