#include "Graph.h"

/***********************************************************
 Fine-grain Function used to create and add AU node in graph
************************************************************/

void Graph::add_node(int A_ID, int ts_AU, Graph_Node **ref)
{
	Graph_Node *pred = verHead, *curr;
	findVWind(&pred, &curr, A_ID);
	if(curr->AU_ID != A_ID) //if vertex node is not in the grpah
	{
		Graph_Node *temp     = new Graph_Node;
		temp->AU_ID          = A_ID;
		temp->ts             = ts_AU;
		temp->in_count       = 0;
		temp->edgeHead->next = temp->edgeTail;//empty edge list

		temp->next.store(curr, memory_order_seq_cst);

		//!!CAS operation if(pred->next == curr) then CAS successful and it sets pred->next = temp;

		while(atomic_compare_exchange_strong( &(pred->next), &curr, temp ) != true)
		{
			findVWind(&pred, &curr, A_ID);

			if(curr->AU_ID == A_ID) 
			{
				*ref = curr;
				return;//if some other thread added this AU_ID vertex node 
			}
			temp->next.store(curr, memory_order_seq_cst);
			*ref = curr;
		}
		*ref = temp;
	}
	if(curr->AU_ID == A_ID)
		*ref = curr;
}

/********************
 find vertex in graph
********************/
void Graph::findVWind(Graph_Node **ver_pre, Graph_Node **ver_curr, int key)
{
	Graph_Node *pre = *ver_pre, *curr = pre->next;
	while(curr != verTail && key < curr->AU_ID)
	{
		pre  = curr;
		curr = curr->next;
	}
	*ver_pre  = pre;
	*ver_curr = curr;
}


/************************************************
 Logic to add edges between atomic unit in graph.
************************************************/
void Graph::add_edge(int from, int to, int from_ts, int to_ts )
{
	Graph_Node *fromRef, *toRef;
	add_node(from, from_ts, &fromRef);                                           //if vertex node "from" is not present in graph, add "from" node
	add_node(to, to_ts, &toRef);                                               //if vertex node "to" is not present in graph, add "to" node

	Graph_Node *ver_pre = verHead, *ver_curr;
	//findVWind(&ver_pre, &ver_curr, from);                              // find "from" node in Grpah; ver_curr points to "from" node
	ver_curr = fromRef;//optimization
	
	//!check for duplicate Edge in "from" vertex edge list
	EdgeNode *edg_pre = ver_curr->edgeHead, *edg_curr, *vcurrEdgTail = ver_curr->edgeTail;
	findEWind(vcurrEdgTail, &edg_pre, &edg_curr, to);
	
	
	if(edg_curr->au_id == to) 
		return;//edge added eariler

	//!--------------------------
	//!Actuall creation of edge
	//!--------------------------

	//!find "to" vertex node in graph:: (1) to increment in_count (because edge go from->to); (2) to store ref to "to" node in EdgeNode
	Graph_Node *refCurr, *refPre = verHead;
	//findVWind(&refPre, &refCurr, to);                                  //refCurr: point to "this.to" node
	refCurr = toRef;
	refCurr->in_count++;                                               //in-count is atomic variable

	//edg_pre = ver_curr->edgeHead;
	//findEWind(vcurrEdgTail, &edg_pre, &edg_curr, to);                  //find window to add edge in edge_list of current node (from);

	EdgeNode *temp_edg  = new EdgeNode;
	temp_edg->au_id     = to;
	temp_edg->ref       = refCurr;

	temp_edg->next.store(edg_curr, memory_order_seq_cst);

	while(atomic_compare_exchange_strong( &(edg_pre->next), &edg_curr, temp_edg ) != true)
	{
		findEWind(vcurrEdgTail, &edg_pre, &edg_curr, to);

		if(edg_curr->au_id == to) 
			break;//if some other thread added this AU_ID edge node 

		temp_edg->next.store(edg_curr, memory_order_seq_cst);
	}

return;
}


/****************************************
 find window to add new edge in edge list
****************************************/
void Graph::findEWind(EdgeNode* edgeTail, EdgeNode **edg_pre, EdgeNode **edg_curr, int key)
{
	EdgeNode *pre  = *edg_pre;
	EdgeNode *curr = pre->next;
	while(curr != edgeTail && key < curr->au_id)
	{
		pre   = curr;
		curr  = curr->next;
	}
	*edg_pre  = pre;
	*edg_curr = curr;
}


/************
 Print Grpah
************/
void Graph::print_grpah()
{
	cout<<"==============================================================\n";
	cout<<"  Grpah Node | In-Degree | Time Stamp | Out Edges (AU_IDs)\n";
	cout<<"==============================================================\n";
	Graph_Node* g_temp = verHead->next;

	while( g_temp != verTail )
	{
		EdgeNode *o_temp = g_temp->edgeHead->next;
		cout<<"\t"+to_string(g_temp->AU_ID)+" \t   "+to_string(g_temp->in_count)+"\t\t "+to_string(g_temp->ts)+" \t";

		while(o_temp != g_temp->edgeTail)
		{
			cout<< "-->"+to_string(o_temp->au_id)+"";
			o_temp = o_temp->next;
		}
		cout<<endl;
		g_temp = g_temp->next;
	}
	cout<<"==============================================================\n";
}
