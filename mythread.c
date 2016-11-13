#include<stdio.h>
#include<stdlib.h>
#include<ucontext.h>
typedef void *MySemaphore;
typedef void *MyThread;
//Structure to represent a Thread context and parent/children
typedef struct Node
{
	ucontext_t* tContext;
	struct Node *next;
	struct Node *parent;
	//struct Node** children;
	int isBlockedOnJoin;
	int isBlockingParent;
	int numActiveChildren;
	

}Node;

//Structure to define a queue. 

typedef struct Queue
{
	Node *first;
	Node *last;
	
}Queue;

typedef struct semaphore{
	int semValue;
	Queue* semQueue;
	struct semaphore* next;
	//struct semaphore* prev;
	
}MySema;

//typedef struct MySema* MySem;
typedef struct semaphoreQueue{
	MySema* first;
	MySema* last;
}semaphoreQueue;

MySema* getFirstSema(semaphoreQueue *q)
{
	if(q->first==NULL) return NULL;
	else return q->first;

}
void enQueueSemQueue(semaphoreQueue *q,MySema *n)
{
//Adding 1st element
	if(q->first == NULL)
	{
		q->first = n;
		q->last = n;
		n->next = NULL;
		return;
	}
//Adding node to the end of the queue	
	q->last->next = n;
	n->next = NULL;
	q->last = n;
	return;
}

void deQueueSemQueueAt(semaphoreQueue *q,MySema *n)
{
	if(q->first == NULL) return;
	MySema *cur = q->first;
	MySema *prev = q->first;
	while(cur!=NULL)
	{
		if(cur==n)
		{
			if(cur==q->first)
			{
				q->first = q->first->next;
				return;
			}

			prev->next = cur->next;
			cur->next = NULL;
			return;
		}
		prev=cur;
		cur=cur->next;
		
	}	

}

static Queue *readyQueue;
static Queue *blockedQueue;
static semaphoreQueue* mySem;
static Queue *zomQueue;
typedef ucontext_t* mythread;
//static mythread checkExit;
static mythread parentProcess;

void enQueue(Queue *q,Node *n)
{
//Adding 1st element
	if(q->first == NULL)
	{
		////puts("Here");
		q->first = n;
		q->last = n;
		n->next = NULL;
		return;
	}
//Adding node to the end of the queue	
	q->last->next = n;
	n->next = NULL;
	q->last = n;
	return;
}

Node* deQueue(Queue *q)
{
//Check if Queue Empty
	if(q->first == NULL) return NULL;
//If only one element
	if(q->first == q->last)
	{	
		Node *front = q->first;
		q->first = NULL;
		q->last = NULL;
		return front;
	}
//Return front and move the queue
	Node *front = q->first;
	q->first = q->first->next;
	return front;
	
}

Node* getFirst(Queue *q)
{
	if(q==NULL) return NULL;	
	if(q->first==NULL) return NULL;
	else 
	{
		////puts("In getFirst");		
		return q->first;
	}
}
void sizeOfQueue(Queue *q)
{
	Node *n = getFirst(q);
	int i =0;
	while(n!=NULL)
	{
		i++;
		n=n->next;
	}
	//printf("Size of Queue:%d\n",i);
	
}
void deQueueAt(Queue *q,Node *n)
{
	if(q->first == NULL) return;
	Node *cur = q->first;
	Node *prev = q->first;
	while(cur!=NULL)
	{
		if(cur==n)
		{
			if(cur==q->first)
			{
				q->first = q->first->next;
				return;
			}

			prev->next = cur->next;
			cur->next = NULL;
			return;
		}
		prev=cur;
		cur=cur->next;
		
	}	



}

void runReadyQueueFIFO()
{
		
	Node* toRun = getFirst(readyQueue);
	if(toRun==NULL) 
	{
		
		setcontext(parentProcess);
	}
	else
	{
			
		setcontext(toRun->tContext);
	}
	
}



Node* returnNode(MyThread thread)
{
	
	Node *current;
//Search in Ready Queue
	current = getFirst(readyQueue);
	while(current != NULL){
		if(current->tContext == thread)
			return current;
		current=current->next;
	}
//Not found in Ready Queue
//Search in Blocked Queue
	current = getFirst(blockedQueue);
	while(current != NULL){
		if(current->tContext == thread)
			return current;
		current=current->next;
	}
//Search in All Semaphore Queues
	MySema* cur = getFirstSema(mySem);
	while(cur!=NULL)
	{
		//Inside semaphore
		current = getFirst(cur->semQueue);
		while(current != NULL)
		{
			if(current->tContext == thread)
				return current;
			current=current->next;
		}
		cur=cur->next;
	}

}

void MyThreadExit()
{
	
	Node* thread2Delete = deQueue(readyQueue);
	Node *parentofDeletedThread = thread2Delete->parent;
	
	if(thread2Delete==NULL)
	{
		return;
	}
	else
	{
		//puts("In Exit");
		if(thread2Delete->isBlockingParent==0)
		{//puts("In Exit 0");
			if(thread2Delete->parent!=NULL)
			{	//puts("Here Also");
				parentofDeletedThread->numActiveChildren--;
				//puts("Here too");
			}			
			
		}
		else if(thread2Delete->isBlockingParent!=0)
		{//puts("In Exit 1");
			if(parentofDeletedThread!=NULL)
			{
				parentofDeletedThread->isBlockedOnJoin--;
				//printf("%d\n",parentofDeletedThread->isBlockedOnJoin);
				if(parentofDeletedThread->isBlockedOnJoin ==0)
				{
					deQueueAt(blockedQueue,parentofDeletedThread);
					enQueue(readyQueue,parentofDeletedThread);
				}
				parentofDeletedThread->numActiveChildren--;
				
			}
			
		}
		if(thread2Delete->numActiveChildren!=0)
		{
			//puts("numActiveChildren");			
			enQueue(zomQueue,thread2Delete);
			//deQueue(readyQueue);
			
		}
		else 
		{
			//puts("else numActiveChildre");			
			//deQueue(readyQueue);
			thread2Delete->tContext->uc_link = NULL;
			free(thread2Delete);
			////puts("In exit7");
		}
			////puts("leave exit");			
			runReadyQueueFIFO();	

	}

}

/*
This routine is called before any other MyThread call. It is invoked only by the Unix process. 
It is similar to invoking MyThreadCreate immediately followed by MyThreadJoinAll. 
The MyThread created is the oldest ancestor of all MyThreads—it is the “main” MyThread. 
This routine can only be invoked once. 
It returns when there are no threads available to run (i.e., the thread ready queue is empty.
*/
void MyThreadInit (void(*start_funct)(void *), void *args)
{
	readyQueue = (Queue*)malloc(sizeof(Queue));
	readyQueue->first = NULL;
	readyQueue->last = NULL;
	blockedQueue = (Queue*)malloc(sizeof(Queue));
	blockedQueue->first = NULL;
	blockedQueue->last = NULL;
	zomQueue = (Queue*)malloc(sizeof(Queue));
	zomQueue->first = NULL;
	zomQueue->last = NULL;	
	parentProcess = (mythread)malloc(sizeof(ucontext_t));
	if(getcontext(parentProcess)==-1) return;
	mySem = (struct semaphoreQueue*)malloc(sizeof(struct semaphoreQueue));	
	mythread rootThread;
	rootThread = (mythread)malloc(sizeof(ucontext_t));
	if(getcontext(rootThread)==-1) return;
	char* root_stack = (char*)malloc(sizeof(char)*8192);
	rootThread->uc_stack.ss_sp = root_stack;
	rootThread->uc_stack.ss_size = sizeof(char)*8192;
	rootThread->uc_link = NULL;
	
	makecontext(rootThread,start_funct,1,args);
	Node* rootNode = (Node*)malloc(sizeof(Node));
	rootNode->next= NULL;
	rootNode->parent = NULL;
	
	rootNode->tContext = rootThread;
	rootNode->isBlockedOnJoin = 0;
	rootNode->isBlockingParent = 0;
	
	enQueue(readyQueue,rootNode);
	//puts("Init");	
	////puts(getFirst(readyQueue)->isBlockedOnJoin);
	swapcontext(parentProcess,rootThread);
	

}

MyThread MyThreadCreate (void(*start_funct)(void *), void *args)
{	////puts("In create");
	mythread new = (mythread)malloc(sizeof(ucontext_t));
	char* new_stack = (char*)malloc(sizeof(char) * 8192);
	if(getcontext(new)==-1)
	{
		//puts("Error in getting context for this thread");
		return NULL;
	}////puts("In create1");
	
	////puts("In create2");	
	new->uc_stack.ss_sp = new_stack;
	new->uc_stack.ss_size = sizeof(char)*8192;
	//if(checkExit!=NULL)
	//new->uc_link = checkExit;
	//else 
	new->uc_link = NULL;
	////puts("In create3");
	makecontext(new,start_funct,1,args);
	Node* newNode = (Node*)malloc(sizeof(Node));
	
	newNode->tContext = new;
	newNode->isBlockedOnJoin = 0;
	newNode->isBlockingParent = 0;
	newNode->numActiveChildren = 0;
	newNode->next = NULL;
	////puts("In create4");
	newNode->parent = getFirst(readyQueue);
	////puts("In create5");
	if(newNode->parent!=NULL) 
	{////puts("Parent Not Null");//puts("After this");
			Node *parentNode = getFirst(readyQueue);
			parentNode->numActiveChildren++;
			//printf("In create, size of children of Parent=%d\n",parentNode->numActiveChildren);
			
	}////puts("In create6");
	enQueue(readyQueue,newNode);
	sizeOfQueue(readyQueue);
	return (MyThread)(new);

}

void MyThreadYield()
{
	Node *runningNode = deQueue(readyQueue);
	runningNode->next = NULL;
	Node* nextOnQueue = getFirst(readyQueue);
	if(nextOnQueue==NULL) 
	{	
		enQueue(readyQueue,runningNode);
		return;
	}
	enQueue(readyQueue,runningNode);
	swapcontext(runningNode->tContext,nextOnQueue->tContext);
}

int MyThreadJoin(MyThread thread)
{
	Node* invokingThread = getFirst(readyQueue);
	Node* nextOnQueue = invokingThread->next;
	Node* paramNode = returnNode((mythread)thread);
	if(paramNode==NULL)
	{
		////puts("Could not find thread. Child exited");
		return -1;
	}
	else
	{
		if(paramNode->parent!= invokingThread)
			return -1;
		else if(paramNode->isBlockingParent==0)
		{
						
			deQueue(readyQueue);			
			invokingThread->isBlockedOnJoin++;;
			paramNode->isBlockingParent = 1;			
			enQueue(blockedQueue,invokingThread);
			if(nextOnQueue!=NULL)
			swapcontext(invokingThread->tContext,nextOnQueue->tContext);
			else setcontext(parentProcess);

		}
		
		
	}
	
}

void MyThreadJoinAll(void)
{	
	
	//puts("In JoinAll");
	sizeOfQueue(readyQueue);
	Node *parentToJoin = getFirst(readyQueue);
	Node *nextOnQueue = parentToJoin->next;
	if(parentToJoin->numActiveChildren==0)
	{
		//puts("No Active cHILEDREN");
		return;
	}
	Node *cur;
	Node*current;
	int numOfchildrenPending = parentToJoin->numActiveChildren;
	int i =0;
		//Search in Ready Queue	
		cur = getFirst(readyQueue);
		while(cur != NULL)
		{	
			if(cur->parent == parentToJoin)
			{
				i++;
				if(cur->isBlockingParent==1);
				else
				{ 	//puts("Here");
					cur->isBlockingParent++;
					//printf("Child blocking count:%d\n",cur->isBlockingParent);
					cur->parent->isBlockedOnJoin++;
					//printf("Parent blocked count:%d\n",cur->parent->isBlockedOnJoin);
					sizeOfQueue(readyQueue);
										
					
				} 
			}
			cur=cur->next;
		}
		////printf("%d\n",i);
		//Search in Blocked Queue
		cur = getFirst(blockedQueue);
		while(cur != NULL)
		{	//puts("Iter BlockedQueue");
			if(cur->parent == parentToJoin)
			{
				i++;
				if(cur->isBlockingParent==1);
				else
				{ 
					cur->isBlockingParent=1;
					cur->parent->isBlockedOnJoin++;					
					
				} 
			}
			cur=cur->next;
		}
		//Search in all Semaphore Queues

		MySema* curr = getFirstSema(mySem);
		while(curr!=NULL)
		{
			//puts("Iter Sema Queue");
			//Inside semaphore
			current = getFirst(curr->semQueue);
			while(current != NULL)
			{
				if(current->parent == parentToJoin)
				{
					i++;
					if(current->isBlockingParent==1);
					else
					{
						current->isBlockingParent=1;
						current->parent->isBlockedOnJoin++;		

					}


				}
				current=current->next;
			}
			curr=curr->next;
		}

		
	
	if(i==0)
	{
		return;
	}
	else
	{
		////puts("Blocking Parent");
		parentToJoin = deQueue(readyQueue);		
		enQueue(blockedQueue,parentToJoin);
		sizeOfQueue(readyQueue);
		////puts("Blocked Parent");
		sizeOfQueue(readyQueue);
		////puts("Size of BlockedQeue");
		sizeOfQueue(blockedQueue);
		if(nextOnQueue!=NULL)
		{
			//puts("Joinn all swap context");	
			sizeOfQueue(readyQueue);
			swapcontext(parentToJoin->tContext,nextOnQueue->tContext);
		}	
		else
		 {
			setcontext(parentProcess);
		 }	
	}

}




// ****** SEMAPHORE OPERATIONS ****** 
// Create a semaphore
MySemaphore MySemaphoreInit(int initialValue)
{
	MySema* mysem = (MySema*)malloc(sizeof(struct semaphore));
	Queue* semQ = (Queue*)malloc(sizeof(Queue));
	mysem->semValue = initialValue;
	mysem->semQueue = semQ;
	mysem->semQueue->first =NULL;
	mysem->semQueue->last = NULL;
	enQueueSemQueue(mySem,mysem);
	return (MySemaphore)(mysem);
	
}

// Signal a semaphore
void MySemaphoreSignal(MySemaphore sem)
{
	//puts("releasing1");	
	MySema* mysem =(MySema*)sem;
	if(mysem==NULL) return;
	mysem->semValue++;
	if(mysem->semValue>=0)
	{
		////puts("releasing");
		Node* waitingThread= deQueue(mysem->semQueue);
		if(waitingThread!=NULL)
		{
			enQueue(readyQueue,waitingThread);
			
		}
	}
	
}

void MySemaphoreWait(MySemaphore sem)
{
	MySema* mysem =(MySema*)sem;
	if(mysem==NULL) return;
	mysem->semValue--;
	if(mysem->semValue < 0)
	{
		//Block;
		Node *toBlock = deQueue(readyQueue);
		enQueue(mysem->semQueue,toBlock);
		if(getFirst(readyQueue)==NULL) setcontext(parentProcess);
		else swapcontext(toBlock->tContext,getFirst(readyQueue)->tContext);
		
	}

}

int MySemaphoreDestroy(MySemaphore sem)
{
	MySema* mysem =(MySema*)sem;
	if(mysem==NULL) return -1;
	if(mysem->semQueue->first!=NULL)
	return -1;
	deQueueSemQueueAt(mySem,mysem);
	free(mysem->semQueue);
	free(mysem);
	return 0;	
}




	



