#ifndef MAIN_ROUTING_H
#define MAIN_ROUTING_H
#include<netdb.h>
#include<sys/types.h>
#include<netinet/in.h>
#include<sys/socket.h>
#include <arpa/inet.h>
#include<string.h>
#include "DistanceVector.h"
#include "Defines.h"

//enum to identify message type received from neighbors
enum MessageType{ROUTINGMESSAGE,UPDATEMESSAGE};


struct ServerList
{
    char IP[16];
    int portNumber;
    int serverID;
};

struct UpdateFormat
{
    int format;
    int serverID;
    int cost;
};

struct NeighborList
{
    int serverID;
    int cost;
};


class DistanceVector;

class Routing
{
    static Routing *server_instance;
    char toplogyFileName[128];
    int routingTimeInterval;
    int numOfServers;
    int numOfNeighbors;
    int selfServerID;
    ServerList sList[5];
    NeighborList nList[5];
    fd_set read_master;
    fd_set read_fds;
    friend class DistanceVector;
    int _indexToServerID[5];
    int packetCount;
    bool updateFromNeighbor[5];
    int updateCountFromNeighbor[5];
    bool serverCrashed;
    Routing():routingTimeInterval(0),numOfServers(0),numOfNeighbors(0),packetCount(0),serverCrashed(0)
    {
        memset(sList,0,sizeof(sList));
        for(int i = 0 ; i < MAXSERVERS ; i++)
        {
            nList[i].cost = INF;
            nList[i].serverID = -1;
        }
        memset(nList,0,sizeof(nList));
        memset(updateFromNeighbor,0,sizeof(bool)*5);
        memset(updateCountFromNeighbor,0,sizeof(int)*5);
    }

    public :
    static Routing *GetInstance()
    {
        if (!server_instance)
          server_instance = new Routing;
        return server_instance;
    }

    void ReadTopologyFile(char *fileName, int time);
    void AddServerDetails(int serverID,char *IP, int portNum , int ind);
    void AddNeighborDetails(int s ,int cost);
    void Manager();
    void ExecuteCommand(char *cmdTokens[4],int numOfTokens);
    void ExecuteUpdate(int s1,int s2,int cost);
    void ExecuteStep();
    void ExecutePackets();
    void ExecuteDisable(int s);
    void ExecuteCrash();
    void ExecuteDisplay();
    void PeriodicUpdateToNeighbors(char *msgCmd);
    int GetServerID();
    void GetFormatedMessage(unsigned char*);
    int GetIndex(int serverID);
    void ReadFormattedMessage(unsigned char *buff ,int msgSize);
    void SendUpdateToNeighbor(int s , int cost);
    MessageType IdentifyRecvMsgType(unsigned char *buff , int nBytes);
    void ReadUpdateMessage(unsigned char *buf);
};


class HelperFunctions
{
    public:
    static bool CheckStringToIntValidity(char *);
    static void ToLowerCase(char *myStr);
    static void TokenizeCommand(char*,char *tokens[3],int *nTokens);
    static const char* IntToString(int num);
    static void PortNumToString(int num,char *port);
    static long long GetCurrentTimeInMS();
    static void ConvertIPtoInteger(char *IP,int intIP[4]);
};


#endif
