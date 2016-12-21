#include "DistanceVector.h"
#include "MainRouting.h"

#include <stdio.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <sys/unistd.h>
#include <iostream>
#include <string.h>
#include <string>
#include <stdlib.h>
#include <string>
#include <sstream>

#include "Defines.h"

using namespace std;


/*
Index to server Id mapping , used to find the index using server id
*/
int DistanceVector::GetIndex(int serverID)
{
    for(int i = 0 ; i < routingObj->numOfServers ; i++)
    {
        if(serverID == indexToServerID[i])
        {
            return i;
        }
    }
}

/*
Initial distance vector is initialized using the intial available
info
*/
void DistanceVector::DistanceVectorInitialization()
{
    routingObj = Routing::GetInstance();

    //Initialize selfDV
    for(int i = 0 ; i < routingObj->numOfServers ; i++)
    {
        if(routingObj->selfServerID != routingObj->sList[i].serverID)
        {
            selfDV[i].cost = INF;
            selfDV[i].serverID = routingObj->sList[i].serverID;
            indexToServerID[i] = selfDV[i].serverID;
        }
        else
        {
            selfDV[i].cost = 0;
            selfDV[i].serverID = routingObj->sList[i].serverID;
            indexToServerID[i] = selfDV[i].serverID;
        }

    }

    //Initialize the neighbor DV matrix
    for(int i = 0 ; i < MAXSERVERS ; i++)
    {
        for(int j = 0 ; j < MAXSERVERS ; j++)
        {
            neighborDV[i][j].cost = INF;
            //The distance of any server to itself is 0
            if(i == j)
            {
                neighborDV[i][j].cost = 0;
            }
        }
    }

    int selfInd = GetIndex(routingObj->selfServerID);

    //Set the initial DV values of the server to its initial neighbor cost links
    for(int j = 0 ; j < routingObj->numOfServers ; j++)
    {
        //Checking if it is a neighbor or not
        if(routingObj->nList[j].cost != 0 && routingObj->nList[j].cost != INF)
        {
            int ind = GetIndex(routingObj->nList[j].serverID);
            neighborDV[selfInd][ind].cost = routingObj->nList[j].cost;
        }

    }
}

/*
Update the neighbor DV cost values in the cost matrix neighbor[][]
*/
void DistanceVector::AddNeighborDVDetails(int serverID1,int serverID2 ,int cost)
{
    int i = GetIndex(serverID1);
    neighborDV[i][GetIndex(serverID2)].cost = cost;
}

/*
Computing DV using Bellman-Ford equation
*/
void DistanceVector::ComputeDistanceVector()
{

    for(int i = 0 ; i < routingObj->numOfServers ; i++)
    {
        selfDV[i].cost = INF;
        selfDV[i].nextHop = -1;
    }

    //The distance vector cost to self is 0 and next hop is itself
    selfDV[GetIndex(routingObj->selfServerID)].cost = 0;
    selfDV[GetIndex(routingObj->selfServerID)].nextHop = routingObj->selfServerID;

   for(int i = 0 ; i < routingObj->numOfServers ; i++)
   {
        //Checking if the destination server is not itself
        if(routingObj->sList[i].serverID != routingObj->selfServerID)
        {
            for(int j = 0 ; j < routingObj->numOfServers; j++)
            {
                //Checking if it is a neighbor or not
                if(routingObj->nList[j].cost != 0 && routingObj->nList[j].cost != INF)
                {
                    int nCost = routingObj->nList[j].cost;
                    //Checking if it the neighbor has path to the destination server
                    if(neighborDV[j][i].cost != INF)
                    {
                        //Bellman-Ford equation
                        selfDV[i].cost = MINIMUM((nCost + neighborDV[j][i].cost),selfDV[i].cost);

                        //Checking if the min cost is updated , if yes next hop is also updated
                        if(selfDV[i].cost == nCost + neighborDV[j][i].cost)
                        {
                            selfDV[i].nextHop = routingObj->nList[j].serverID;
                        }
                    }
                }
            }
        }

   }

}

DistanceVector *DistanceVector::dv_instance = 0;
