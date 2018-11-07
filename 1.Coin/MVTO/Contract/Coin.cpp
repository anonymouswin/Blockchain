/********************************************************************
|  *****     File:   Contract.cpp                            *****  |
|  *****  Created:   July 23, 2018, 01:46 AM                 *****  |
********************************************************************/

#include "Coin.h"



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

//	if(amount > account[sender_iD]) return false; //not sufficent balance to send; AU is invalid.
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
		cout<<"\nERROR:: Only ''MINTER'' (Contract Creator) can initialize the Accounts (Shared Objects)\n";
		return false;//false not a minter.
	}
	
	long long error_id;
	trans_state* T       = lib->begin();//Transactions begin.
	*time_stamp          = T->TID;      //return time_stamp to user.
	common_tOB* rec_obj  = new common_tOB;
	rec_obj->size        = sizeof(int);
	rec_obj->ID          = receiver_iD;
			
	if( lib->read(T, rec_obj) != SUCCESS)//read into local tob for sender_iD.
		return false;//AU aborted.

	*(int*)rec_obj->value = *(int*)rec_obj->value + amount;
	if(lib->write(T, rec_obj) != SUCCESS ) 
	{
		lib->try_abort( T );
		return false;//AU aborted.
	}

	if(lib->try_commit(T, error_id) == SUCCESS)	
		return true;//AU execution successful.
	else
		return false;//AU aborted.
}


int Coin::send_m(int t_ID, int sender_iD, int receiver_iD, int amount, int *time_stamp, list<int>&conf_list) 
{
	long long error_id;	
	trans_state* T         = lib->begin();//Transactions begin.
	*time_stamp            = T->TID;      //return time_stamp to user.
	
	common_tOB* sender_obj = new common_tOB;
	sender_obj->size       = sizeof(int);
	sender_obj->ID         = sender_iD;
	common_tOB* rec_obj    = new common_tOB;
	rec_obj->size          = sizeof(int);
	rec_obj->ID            = receiver_iD;
			
	if(lib->read(T, sender_obj ) != SUCCESS || lib->read(T, rec_obj) != SUCCESS) 
		return 0;//AU aborted.
				
	else if(amount > *(int*)sender_obj->value )
	{
		lib->try_abort( T );
		return -1;//not sufficent balance to send; AU is invalid, Trans aborted.
	}
	
	*(int*)sender_obj->value  = *(int*)sender_obj->value - amount;
	*(int*)rec_obj->value     = *(int*)rec_obj->value + amount;
		
	if(lib->write(T, sender_obj) != SUCCESS || lib->write(T, rec_obj) != SUCCESS) 
	{
		lib->try_abort( T );
		return 0;//AU aborted.
	}
	
	if(lib->try_commit_conflict(T, error_id, conf_list) == SUCCESS ) 
		return 1;//send done; AU execution successful.
	else
		return 0;//AU aborted.
}
	

bool Coin::get_bal_m(int account_iD, int *bal, int t_ID, int *time_stamp, list<int>&conf_list) 
{
	long long error_id;
	trans_state* T      = lib->begin(); //Transactions begin.
	*time_stamp         = T->TID;       //return time_stamp to user.
	common_tOB* acc_bal = new common_tOB;
	acc_bal->size       = sizeof(int);
	acc_bal->ID         = account_iD;
		
	if(lib->read(T, acc_bal) != SUCCESS) 
		return false;//AU aborted.
	*bal = *(int*)acc_bal->value;
	
	if(lib->try_commit_conflict(T, error_id, conf_list) == SUCCESS) 
		return true;//AU execution successful.
	else 
		return false;//AU aborted.
}
		
