#include <cstdio>
#include <cstring>
#include <stdlib.h>
#include "GetTime.h"
#include "Rand.h"
#include "RakPeerInterface.h"
#include "MessageIdentifiers.h"
#include "RakNetworkFactory.h"
#include "ReadyEvent.h"
#include <assert.h>
#include <conio.h>
#include "RakSleep.h"
#include "FullyConnectedMesh.h"

void PrintConnections();

static const int NUM_PEERS=3;
RakPeerInterface *rakPeer[NUM_PEERS];
ReadyEvent readyEventPlugin[NUM_PEERS];
FullyConnectedMesh fullyConnectedMeshPlugin[NUM_PEERS];

int main(void)
{
	int i;

	for (i=0; i < NUM_PEERS; i++)
		rakPeer[i]=RakNetworkFactory::GetRakPeerInterface();

	printf("This project tests and demonstrates the ready event plugin.\n");
	printf("It is used in a peer to peer environment to have a group of\nsystems signal an event.\n");
	printf("It is useful for changing turns in a turn based game,\nor for lobby systems where everyone has to set ready before the game starts\n");
	printf("Difficulty: Beginner\n\n");

	int peerIndex;

	// Initialize the message handlers
	for (peerIndex=0; peerIndex < NUM_PEERS; peerIndex++)
	{
		fullyConnectedMeshPlugin[peerIndex].Startup(0);
		rakPeer[peerIndex]->AttachPlugin(&fullyConnectedMeshPlugin[peerIndex]);
		rakPeer[peerIndex]->AttachPlugin(&readyEventPlugin[peerIndex]);
		rakPeer[peerIndex]->SetMaximumIncomingConnections(NUM_PEERS);
	}

	// Initialize the peers
	for (peerIndex=0; peerIndex < NUM_PEERS; peerIndex++)
	{
		SocketDescriptor socketDescriptor(60000+peerIndex,0);
		rakPeer[peerIndex]->Startup(NUM_PEERS, 0, &socketDescriptor, 1);
	}

	// Give the threads time to properly start
	RakSleep(200);

	printf("Peers initialized. ");
	printf("Connecting each peer to the prior peer\n");

	for (peerIndex=0; peerIndex < NUM_PEERS; peerIndex++)
	{
		unsigned peerIndex2;
		for (peerIndex2=0; peerIndex2<NUM_PEERS; peerIndex2++)
		{
			if (peerIndex2!=peerIndex)
			{
				rakPeer[peerIndex]->Connect("127.0.0.1", 60000+peerIndex2, 0, 0);
			}
		}
	}

	// Let the connections be active
	RakSleep(500);

	PrintConnections();

	for (peerIndex=0; peerIndex < NUM_PEERS; peerIndex++)
	{
		readyEventPlugin[peerIndex].AddToWaitList(0, UNASSIGNED_SYSTEM_ADDRESS);
	}

	printf("All systems should now be waiting on event 0 from all other systems\n");
	printf("'S' to signal a system\n");
	printf("'U' to unsignal a system\n");
	printf("'Q' to quit\n");
	printf("' ' to print wait statuses\n");

	char str[128];
	char ch=0;
	while (1)
	{
		if (kbhit())
			ch=getch();

		if (ch=='s' || ch=='S')
		{
			ch=0;
			printf("Which system? 0 to %i\n", NUM_PEERS-1);
			gets(str);
			int sysIndex = atoi(str);
			if (sysIndex>=0 && sysIndex<NUM_PEERS)
			{
				if (readyEventPlugin[sysIndex].SetEvent(0,true))
					printf("Set system %i to signaled\n", sysIndex);
				else
					printf("Set system %i to signaled FAILED\n", sysIndex);
			}
			else
			{
				printf("Invalid range\n");
			}
		}
		if (ch=='u' || ch=='U')
		{
			ch=0;
			printf("Which system? 0 to %i\n", NUM_PEERS-1);
			gets(str);
			int sysIndex = atoi(str);
			if (sysIndex>=0 && sysIndex<NUM_PEERS)
			{
				if (readyEventPlugin[sysIndex].SetEvent(0,false))
					printf("Set index %i to unsignaled\n", sysIndex);
				else
					printf("Set index %i to unsignaled FAILED\n", sysIndex);
			}
			else
			{
				printf("Invalid range\n");
			}
		}
		if (ch==' ')
		{
			SystemAddress sysAddr;
			sysAddr.SetBinaryAddress("127.0.0.1");
			unsigned j;
			printf("\n");
			for (i=0; i < NUM_PEERS; i++)
			{
				printf("System %i, ", i);
				if (readyEventPlugin[i].IsEventSet(0))
					printf("Set=True, ");
				else
					printf("Set=False, ");

				if (readyEventPlugin[i].IsEventCompleted(0))
					printf("Completed=True\n");
				else if (readyEventPlugin[i].IsEventCompletionProcessing(0))
					printf("Completed=InProgress\n");
				else
					printf("Completed=False\n");

				for (j=0; j < NUM_PEERS; j++)
				{
					if (i!=j)
					{

						ReadyEventSystemStatus ress;
						sysAddr.port=60000+j;
						ress = readyEventPlugin[i].GetReadyStatus(0, sysAddr);
						printf("  Remote system %i, status = ", j);
						
						switch (ress)
						{
						case RES_NOT_WAITING:
							printf("RES_NOT_WAITING\n");
							break;
						case RES_WAITING:
							printf("RES_WAITING\n");
							break;
						case RES_READY:
							printf("RES_READY\n");
							break;
						case RES_ALL_READY:
							printf("RES_ALL_READY\n");
							break;
						case RES_READY_NOT_WAITING:
							printf("RES_READY_NOT_WAITING\n");
							break;
						case RES_ALL_READY_NOT_WAITING:
							printf("RES_ALL_READY_NOT_WAITING\n");
							break;
						case RES_UNKNOWN_EVENT:
							printf("RES_UNKNOWN_EVENT\n");
							break;

						}
					}
				}
			}
			ch=0;
		}
		if (ch=='Q')
		{
			break;
		}

		for (i=0; i < NUM_PEERS; i++)
		{
			rakPeer[i]->DeallocatePacket(rakPeer[i]->Receive());
		}

		// Keep raknet threads responsive
		RakSleep(30);
	}
	
	for (i=0; i < NUM_PEERS; i++)
		RakNetworkFactory::DestroyRakPeerInterface(rakPeer[i]);

	return 1;
}

void PrintConnections()
{
	int i,j;
	char ch=0;
	SystemAddress systemAddress;
	Packet *packet;
	printf("Connecting.  Press space to see status or c to continue.\n");

	while (ch!='c' && ch!='C')
	{
		if (kbhit())
			ch=getch();

		if (ch==' ')
		{
			printf("--------------------------------\n");
			for (i=0; i < NUM_PEERS; i++)
			{
				if (NUM_PEERS<=10)
				{
					/*
					printf("%i (Mesh): ", 60000+i);
					for (j=0; j < (int)fullyConnectedMeshPlugin[i].GetMeshPeerListSize(); j++)
					{
					systemAddress=fullyConnectedMeshPlugin[i].GetPeerIDAtIndex(j);
					if (systemAddress!=UNASSIGNED_SYSTEM_ADDRESS)
					printf("%i ", systemAddress.port);
					}

					printf("\n");
					*/

					printf("%i (Conn): ", 60000+i);
					for (j=0; j < NUM_PEERS; j++)
					{
						systemAddress=rakPeer[i]->GetSystemAddressFromIndex(j);
						if (systemAddress!=UNASSIGNED_SYSTEM_ADDRESS)
							printf("%i ", systemAddress.port);
					}

					printf("\n");
				}
				else
				{
					int connCount;
					//int meshCount;
					for (connCount=0, j=0; j < NUM_PEERS; j++)
					{
						systemAddress=rakPeer[i]->GetSystemAddressFromIndex(j);
						if (systemAddress!=UNASSIGNED_SYSTEM_ADDRESS)
							connCount++;
					}
					/*
					for (meshCount=0, j=0; j < (int)fullyConnectedMeshPlugin[i].GetMeshPeerListSize(); j++)
					{
					systemAddress=fullyConnectedMeshPlugin[i].GetPeerIDAtIndex(j);
					if (systemAddress!=UNASSIGNED_SYSTEM_ADDRESS)
					meshCount++;
					}
					*/

					//printf("%i (Mesh): %i peers should be connected\n", 60000+i, meshCount);
					printf("%i (Conn): %i peers are connected\n", 60000+i, connCount);
				}
			}
			printf("\n");
			ch=0;

			printf("--------------------------------\n");
		}

		for (i=0; i < NUM_PEERS; i++)
		{
			packet=rakPeer[i]->Receive();
			if (packet)
				rakPeer[i]->DeallocatePacket(packet);
		}

		// Keep raknet threads responsive
		RakSleep(30);
	}	
}