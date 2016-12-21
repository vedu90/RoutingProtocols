#include <sys/time.h>
#include <stdlib.h>
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
#include <fstream>
#include <algorithm>
#include "MainRouting.h"
#include "Defines.h"
#include "DistanceVector.h"

using namespace std;


/*
Gets the currentTime in milliseconds
*/
long long HelperFunctions::GetCurrentTimeInMS()
{
    struct timeval currentTime;
    gettimeofday(&currentTime, NULL);
    long long currentMS = (long long)currentTime.tv_sec * 1000 + currentTime.tv_usec / 1000;
    return currentMS;
}

/*
Coverts port number from integer to char*
*/
void HelperFunctions::PortNumToString(int num,char *port)
{
    int temp = num;
    int len = 0;
    while(temp != 0)
    {
        temp = temp/10;
        len++;
    }

    int k = len-1;
    temp = num;
    while(temp != 0)
    {
        int rem = temp%10;
        temp = temp/10;
        port[k] = rem+48;
        k--;
    }
    port[len] = '\0';
}

/*
Converts the given argument to lower case
*/
void HelperFunctions::ToLowerCase(char *myStr)
{
    for (unsigned int i = 0; i < strlen(myStr); i++)
    {
        if (std::isalpha(myStr[i]))
        {
            myStr[i] = std::tolower(myStr[i]);
        }
    }
}


/*
Tokenizes the argument into a max of 4 tokens using space as delimiter
*/
void HelperFunctions::TokenizeCommand(char *myStr,char *tokens[3],int *nTokens)
{
    char *p = strtok(myStr, " ");
    int i = 0;
    int n  = 0;
    while(p!= NULL && i < 4)
    {
        tokens[i] = p;
        p = strtok(NULL," ");
        i++;
        n++;
    }
    *nTokens = n;
}

/*
    Checks if the given port number is a valid number
*/
bool HelperFunctions::CheckStringToIntValidity(char *portNumber)
{
    for (unsigned int i = 0; i < strlen (portNumber); i++)
    {
        if (!std::isdigit(portNumber[i]))
        {
            return false;
        }
    }
    return true;
}


/*
    Converts the char * IP  into integer array of size 4
    e.g. "107.108.192.207" is converted into int array {107,108,192,207}
*/
void HelperFunctions::ConvertIPtoInteger(char *IP,int intIP[4])
{
        for(unsigned int i = 0 ; i < strlen(IP) ; i++)
        {
                if(*(IP+i) == '.')
                {
                        IP[i] = ' ';
                }
        }

        std::string str(IP);
        std::istringstream iss(str);

        iss>>intIP[0]>>intIP[1]>>intIP[2]>>intIP[3];

}

void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET)
    {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }
}


/*
This API is called whenever the server receives a cost update message from
one of its neighbors
*/
void Routing::ReadUpdateMessage(unsigned char *buf)
{
    UpdateFormat updateMsg;
    memcpy(&updateMsg,buf,sizeof(UpdateFormat));

    if(updateMsg.format != 0xFFFFFFFF)
    {
        cout<<"Error!!! its not a cost update message";
        return;
    }

    int ind = GetIndex(updateMsg.serverID);

    nList[ind].cost = updateMsg.cost;

    cout<<"Received update Message with cost "<<updateMsg.cost<<" from server ID "<<updateMsg.serverID<<"\n";

    DistanceVector::GetInstance()->ComputeDistanceVector();
}

/*
Identifies whether the received message is a cost update message or Distance vector message
UPDATEMESSAGE - cost update message
ROUTINGMESSAGE - Distance vector message
*/
MessageType Routing::IdentifyRecvMsgType(unsigned char *buff,int nBytes)
{
    if(nBytes == sizeof(UpdateFormat))  //Size of cost update message is always equal to sizeof(UpdateFormat) and
    {                                   //its size is always less than DV message. Also there is an additional
  //      cout<<"Message Type : Update\n";    //condition in ReadUpdateMessage() to check whether it is
        return UPDATEMESSAGE;               //actually a cost update message
    }
    else
    {
   //     cout<<"Message Type : Routing\n";
        return ROUTINGMESSAGE;
    }

}

/*
The distance vector message received by the server is in the specified format of the document
It should be read byte by byte and respective distance vector of the sender server should be updated
*/
void Routing::ReadFormattedMessage(unsigned char *buff,int msgSize)
{
    unsigned int numOfUpdateFields;
    int offset = 0;
    numOfUpdateFields = buff[offset++];
    numOfUpdateFields += 256*buff[offset++];
    int expectedSize = (8+numOfUpdateFields*12);

    if(msgSize != expectedSize)
    {
        printf("Error!!! Complete message not received, expecting %d, actual received %d\n",expectedSize,msgSize);
        return;
    }

    unsigned int senderPort;
    senderPort = buff[offset++];
    senderPort += 256*buff[offset++];

    unsigned char senderIP[5];

    senderIP[0] = buff[offset++];
    senderIP[1] = buff[offset++];
    senderIP[2] = buff[offset++];
    senderIP[3] = buff[offset++];
    senderIP[4] = '\0';

    offset += 8;
    unsigned int *serverID = new unsigned int[numOfUpdateFields];
    unsigned int *cost = new unsigned int[numOfUpdateFields];
    unsigned int senderServerID;
    for(unsigned int i = 0 ; i < numOfUpdateFields ; i++)
    {
        serverID[i] = buff[offset++];
        serverID[i] += 256*buff[offset++];

        cost[i] = buff[offset++];
        cost[i] += 256*buff[offset++];

        if(cost[i] == 0)
        {
            senderServerID = serverID[i];
        }
        offset += 8;
    }

    //If the message is from a neighbor server whose cost is INF , ignore it
    if(nList[GetIndex(senderServerID)].cost == INF)
    {
        cout<<"Connection with neighbor server ID "<<senderServerID<<" has been closed , ignore the msg from that server\n";
        return;
    }

    cout<<"RECEIVED A MESSAGE FROM SERVER "<<senderServerID<<endl;
    packetCount++;      //Increment the packet count received by 1
    updateFromNeighbor[GetIndex(senderServerID)] = true;    //Set recevied flag from that server to true

    //Update DV of the sender server
    for(unsigned int i = 0 ; i < numOfUpdateFields ; i++)
    {
        DistanceVector::GetInstance()->AddNeighborDVDetails(senderServerID,serverID[i],cost[i]);
    }

    //Compute the distance vector after the above update of DV
    DistanceVector::GetInstance()->ComputeDistanceVector();
}


/*
Builds the message in the update message format before sending it to neighbor
*/
void Routing::GetFormatedMessage(unsigned char* buffMsg)
{
    int numOfUpdateFields = numOfServers;
    int offset = 0;

    buffMsg[offset++] = 0x00FF & numOfUpdateFields;
    buffMsg[offset++] = (0xFF00 & numOfUpdateFields)>>8;

    int portNum = sList[GetIndex(selfServerID)].portNumber;

    buffMsg[offset++] = 0x00FF & portNum;
    buffMsg[offset++] = (0xFF00 & portNum)>>8;

    char _IP[16];
    strcpy(_IP,sList[GetIndex(selfServerID)].IP);
    int intIP[4] = {0,};
    HelperFunctions::ConvertIPtoInteger(_IP,intIP);
    buffMsg[offset++] = 0xFF & intIP[0];
    buffMsg[offset++] = 0xFF & intIP[1];
    buffMsg[offset++] = 0xFF & intIP[2];
    buffMsg[offset++] = 0xFF & intIP[3];

    for(int i = 0 ; i < numOfServers ; i++)
    {
        memset(_IP,0,strlen(sList[i].IP));
        strcpy(_IP,sList[i].IP);
        memset(intIP,0,sizeof(int)*4);
        HelperFunctions::ConvertIPtoInteger(_IP,intIP);

        buffMsg[offset++] = 0xFF & intIP[0];
        buffMsg[offset++] = 0xFF & intIP[1];
        buffMsg[offset++] = 0xFF & intIP[2];
        buffMsg[offset++] = 0xFF & intIP[3];


        portNum = sList[i].portNumber;

        buffMsg[offset++] = 0x00FF & portNum;
        buffMsg[offset++] = (0xFF00 & portNum)>>8;

        buffMsg[offset++] = 0x00;
        buffMsg[offset++] = 0x00;


        int sID = sList[i].serverID;
        buffMsg[offset++] = 0x00FF & sID;
        buffMsg[offset++] = (0xFF00 & sID)>>8;

        int cost = 0;
        cost = DistanceVector::GetInstance()->selfDV[i].cost;

        buffMsg[offset++] = 0x00FF & cost;
        buffMsg[offset++] = (0xFF00 & cost)>>8;

    }
}

/*
Send distance vector update in the given update message format to
all the neighbors
*/
void Routing::PeriodicUpdateToNeighbors(char *cmdMsg)
{
    int buffSize = 8+numOfServers*12;
    unsigned char* buffMsg = new unsigned char[buffSize+1];

    GetFormatedMessage(buffMsg);
    bool nFlag = false;

    for(int i = 0 ; i < numOfServers ; i++)
    {
        //Checking if it is a neighbor or not
        if(nList[i].cost != 0 && nList[i].cost != INF)
        {
            nFlag = true;
            int sockfd;
            struct addrinfo hints, *servinfo, *p;
            int rv;
            int numbytes;

            memset(&hints, 0, sizeof hints);
            hints.ai_family = AF_UNSPEC;
            hints.ai_socktype = SOCK_DGRAM;

            int portNum = sList[i].portNumber;
            char port[6] = {0,};
            HelperFunctions::PortNumToString(portNum,port);

            if ((rv = getaddrinfo(sList[i].IP, port, &hints, &servinfo)) != 0)
            {
                fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
                return;
            }

            for(p = servinfo; p != NULL; p = p->ai_next)
            {
                if ((sockfd = socket(p->ai_family, p->ai_socktype,p->ai_protocol)) == -1)
                {
                    char msg[40];
                    strcpy(msg,cmdMsg);
                    strcat(msg,": socket ");
                    perror(msg);
                    continue;
                }
                break;
            }

            if (p == NULL)
            {
                fprintf(stderr, "UPDATE: failed to create socket\n");
                return;
            }

            if ((numbytes = sendto(sockfd,buffMsg, buffSize, 0,p->ai_addr, p->ai_addrlen)) == -1)
            {
                char msg[40];
                strcpy(msg,cmdMsg);
                strcat(msg,":sendto ");
                perror(msg);
                close(sockfd);
                exit(1);
            }

            freeaddrinfo(servinfo);
            close(sockfd);
            cout<<"Periodic Update Message sent to server ID :"<<sList[i].serverID<<endl;
        }
    }

    if(nFlag == true)
    {
        cout<<cmdMsg<<" SUCCESS\n";
    }
    else
    {
        cout<<"No neighbors to send periodic update message\n";
    }

    return;
}

/*
Identify the server id of itself using its hostname
*/
int Routing::GetServerID()
{

    char hostName[32];
    char hostIP[16];
    if(!gethostname(hostName,32))
    {
        struct hostent *temp;
        if((temp = gethostbyname(hostName)) != NULL)
        {
            struct in_addr **addr_list;
            addr_list = (struct in_addr **)temp->h_addr_list;
            strcpy(hostIP,inet_ntoa(*addr_list[0]));

        }
        else
        {
            perror("gethostbyname");
        }

    }
    else
    {
        perror("gethostname");
    }

    int i = 0;
    while(i < numOfServers)
    {
        if(strcmp(hostIP,sList[i].IP) == 0)
        {
            return sList[i].serverID;
        }
        i++;
    }

    return 0;
}

bool cmpFunction(DVTable temp1,DVTable temp2)
{
    return temp1.serverID<temp2.serverID;
}

/*
Send cost update message to neighbor whenever cost of a link is modified
*/
void Routing::SendUpdateToNeighbor(int s , int cost)
{
    int sockfd;
    struct addrinfo hints, *servinfo, *p;
    int rv;
    int numbytes;

    UpdateFormat updateMsg;
    updateMsg.format = 0xFFFFFFFF;          //Used to identify whether it is a Cost update message or not
    updateMsg.serverID = selfServerID;
    updateMsg.cost = cost;

    char *buffMsg = new char[sizeof(UpdateFormat)];

    memcpy(buffMsg,&updateMsg,sizeof(UpdateFormat));

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;
    int ind = GetIndex(s);
    int portNum = sList[ind].portNumber;
    char port[6] = {0,};
    HelperFunctions::PortNumToString(portNum,port);

    if ((rv = getaddrinfo(sList[ind].IP, port, &hints, &servinfo)) != 0)
    {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return;
    }

    for(p = servinfo; p != NULL; p = p->ai_next)
    {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,p->ai_protocol)) == -1)
        {
            perror("Cost update: socket");
            continue;
        }
        break;
    }

    if (p == NULL)
    {
        fprintf(stderr, "Cost update: failed to create socket\n");
        return;
    }

    if ((numbytes = sendto(sockfd,buffMsg, sizeof(UpdateFormat), 0,p->ai_addr, p->ai_addrlen)) == -1)
    {
        perror("Cost update: sendto");
        close(sockfd);
        exit(1);
    }

    freeaddrinfo(servinfo);
   // printf("talker: sent %d bytes to %s\n", numbytes, sList[ind].IP);
    cout<<"Cost update message sent to server id "<<sList[ind].serverID<<"\n";
    close(sockfd);
    delete buffMsg;
}

/*
Display all the Destination Server ID's , NexHop Server ID's and their cost of paths
Before displaying, it is sorted according to the server Id
*/
void Routing::ExecuteDisplay()
{
    cout<<"DestinationServer ID   NextHopServer ID   Cost of Path\n";

    DVTable temp[5] ;

    for(int i = 0 ; i < numOfServers ; i++)
    {
        temp[i].cost = DistanceVector::GetInstance()->selfDV[i].cost;
        temp[i].nextHop = DistanceVector::GetInstance()->selfDV[i].nextHop;
        temp[i].serverID = DistanceVector::GetInstance()->selfDV[i].serverID;
    }

    std::sort(temp,temp+numOfServers,cmpFunction);

    for(int i = 0 ; i < numOfServers ; i++)
    {
       cout<<"\t\t"<<temp[i].serverID<<"\t\t"<<temp[i].nextHop<<"\t\t"<<temp[i].cost<<"\n";
    }
    cout<<"DISPLAY SUCCESS\n";
}

/*
Set all the neighbor costs to INF
And then exit()
exit is called in Manager after the execution of this API where the listening socket is closed first
*/
void Routing::ExecuteCrash()
{
    for(int ind = 0 ; ind < numOfServers ; ind++)
    {
        if(nList[ind].cost != 0 && nList[ind].serverID != 0 && nList[ind].serverID != selfServerID)
        {
            nList[ind].cost = INF;
        }
    }
    serverCrashed = true;
    DistanceVector::GetInstance()->ComputeDistanceVector();
    cout<<"CRASH SUCCESS\n";
    cout<<"Exiting...\n";
}

/*
Sets the neighbor link cost to INF
*/
void Routing::ExecuteDisable(int s)
{
    int ind = GetIndex(s);

    //Check whether it is a neighbor or not
    if(nList[ind].cost != 0 && nList[ind].serverID != 0 && s != selfServerID)
    {
        nList[ind].cost = INF;
    }
    else
    {
        cout<<"Error!!! The given Server ID is not a neighbor\n";
        return;
    }
    DistanceVector::GetInstance()->ComputeDistanceVector();
    cout<<"DISABLE SUCCESS\n";
}

/*
Prints the number of packets received since the last time this command was given
*/
void Routing::ExecutePackets()
{
    cout<<"Number of packets received : "<<packetCount<<"\n";
    packetCount= 0;
    cout<<"PACKETS SUCCESS\n";
}

/*
    Triggers the server to send its DV to all the neighbors
*/
void Routing::ExecuteStep()
{
    char msg[] = "STEP";
    PeriodicUpdateToNeighbors(msg);
}

/*
Updates the neighbors link cost and sends the cost update message to the other neighbor
DV is computed afterwards
*/
void Routing::ExecuteUpdate(int s1,int s2,int cost)
{
    nList[GetIndex(s2)].cost = cost;
    SendUpdateToNeighbor(s2,cost);
    DistanceVector::GetInstance()->ComputeDistanceVector();
    cout<<"UPDATE SUCCESS\n";
}

/*
All the user input commands are forwarded to this API which calls the respective API to be called depending
on the command type
*/
void Routing::ExecuteCommand(char *cmdTokens[4] ,int numOfTokens)
{
    if(strcmp(cmdTokens[0],"update") == 0)
    {
        if(numOfTokens != 4)
        {
            cout<<"Error!!! expecting 4 arguments\n";
            return;
        }
        cout<<"Update Command Execution Started\n";
        int s1 = atoi(cmdTokens[1]);
        int s2 = atoi(cmdTokens[2]);
        int cost = 0;
        if(strcmp(cmdTokens[3],"inf") == 0)
        {
            cost = INF;
        }
        else
        {
            cost = atoi(cmdTokens[3]);
        }
        ExecuteUpdate(s1,s2,cost);
    }
    else if(strcmp(cmdTokens[0],"step") == 0)
    {
        cout<<"Step Command Execution Started\n";
        ExecuteStep();
    }
    else if(strcmp(cmdTokens[0],"packets") == 0)
    {
        cout<<"Packets Command Execution Started\n";
        ExecutePackets();
    }
    else if(strcmp(cmdTokens[0],"disable") == 0)
    {
        if(numOfTokens != 2)
        {
            cout<<"Error!!! expecting 2 arguments\n";
            return;
        }
        cout<<"Disable Command Execution Started\n";
        int s = atoi(cmdTokens[1]);
        ExecuteDisable(s);

    }
    else if(strcmp(cmdTokens[0],"crash") == 0)
    {
        cout<<"Crash Command Execution Started\n";
        ExecuteCrash();

    }
    else if(strcmp(cmdTokens[0],"display") == 0)
    {
        cout<<"Display Command Execution Started\n";
        ExecuteDisplay();
    }
    else
    {
        cout<<"Invalid Command Executed\n";
    }
}

/*
This is the main API that controls the program.
UDP listening socket is created here
It simultaneosuly handles STDIN inputs and UDP messages from
other servers using select() used inside this API
*/
void Routing::Manager()
{
    int sockfd;
    struct addrinfo hints, *servinfo, *p;
    int rv;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;
    int port = sList[GetIndex(selfServerID)].portNumber;
    char portNum[6] = {0,};
    HelperFunctions::PortNumToString(port,portNum);

    if ((rv = getaddrinfo(NULL, portNum, &hints, &servinfo)) != 0)
    {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return;
    }

    for(p = servinfo; p != NULL; p = p->ai_next)
    {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,p->ai_protocol)) == -1)
        {
            perror("listener: socket");
            continue;
        }
        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1)
        {
            close(sockfd);
            perror("listener: bind");
            continue;
        }
        break;
    }
    if (p == NULL)
    {
        fprintf(stderr, "listener: failed to bind socket\n");
        return;
    }
    freeaddrinfo(servinfo);

    struct sockaddr_storage their_addr;

    unsigned char buf[BUFF_LEN];
    int fdmax;
    socklen_t addr_len;

    FD_ZERO(&read_master);
    FD_ZERO(&read_fds);

    //Adding STDIN
    FD_SET(0, &read_master);

    FD_SET(sockfd, &read_master);
    fdmax = sockfd;

    struct timeval tv;

    tv.tv_sec = routingTimeInterval;
    tv.tv_usec = 0;

    long long prevTime = HelperFunctions::GetCurrentTimeInMS();
    long long diffInterval = 0;

    for(;;)
    {
        read_fds = read_master;

        if (select(fdmax+1, &read_fds, NULL, NULL, &tv) == -1)
        {
            perror("select");
            exit(4);
        }

       for(int k = 0; k <= fdmax; k++)
        {
            if (FD_ISSET(k, &read_fds))
            {

                if(k == 0)  //STDIN input
                {
                    std::string strCommand;
                    getline(cin,strCommand);
                    char *commandTokens[4] = {NULL,NULL,NULL,NULL};
                    int nTokens = 0;
                    HelperFunctions::TokenizeCommand((char *)strCommand.c_str(),commandTokens,&nTokens);
                    HelperFunctions::ToLowerCase(commandTokens[0]);
                    ExecuteCommand(commandTokens,nTokens);
                    if(serverCrashed == true)   //If server crashed, exit the program
                    {
                        FD_CLR(sockfd,&read_master);
                        FD_CLR(sockfd,&read_fds);
                        close(sockfd);

                        FD_CLR(0,&read_master);
                        FD_CLR(0,&read_fds);
                        close(0);

                        exit(0);
                    }
                }
                else    //if the listening UDP socket receives message from any server
                {
                    int numbytes;
                    memset(buf,0,BUFF_LEN);
                    if((numbytes = recvfrom(sockfd,(void*)buf, BUFF_LEN-1 , 0,(struct sockaddr *)&their_addr, &addr_len)) == -1)
                    {
                        perror("recvfrom");
                        exit(1);
                    }

                    buf[numbytes] = '\0';

                    if(serverCrashed == true)
                    {
                        cout<<"Server crashed , can't take messages from other servers\n";
                        continue;
                    }

                    //Identify the type of msg received
                    if(IdentifyRecvMsgType(buf,numbytes) == ROUTINGMESSAGE)
                    {
                        ReadFormattedMessage(buf,numbytes);
                    }
                    else
                    {
                        ReadUpdateMessage(buf);
                    }
                }
            }

            long long currentTime = HelperFunctions::GetCurrentTimeInMS();
            diffInterval += currentTime - prevTime;

            //Checking whether time interval is completed or not
            if(diffInterval >= routingTimeInterval*1000)
            {
                 //Checking whether all the neighbors have sent
                //message during that particular time interval
                for(int i = 0 ; i < numOfServers ; i++)
                {
                    //Checking whether it is a neighbor or not
                    if(nList[i].cost != 0 && nList[i].cost != INF)
                    {
                        //If they have not sent , increment updateCountFromNeighbor[]
                        //count of that particular server ID
                        if(updateFromNeighbor[i] == false)
                        {
                            updateCountFromNeighbor[i]++;
                            //If the count is more than or equal to3
                            //remove it from the neighbor list by setting
                            //the neighbor link cost to INF
                            if(updateCountFromNeighbor[i] >= 3)
                            {
                                  nList[i].cost = INF;
                                  cout<<"Oops!!! Missed 3 consecutive updates from server ID "<<sList[i].serverID<<\
                                  ", disable connection with that server\n";
                                  DistanceVector::GetInstance()->ComputeDistanceVector();
                            }
                        }
                        //If the neighbor has sent , set the flag to false again and
                        //count to 0
                        else
                        {
                            updateFromNeighbor[i] = false;
                            updateCountFromNeighbor[i] = 0;
                        }
                    }
                }

                //Send the periodic update message
                char _pMsg[] = "PERIODIC UPDATE";
                PeriodicUpdateToNeighbors(_pMsg);

                //setting the time interval again for select()
                tv.tv_sec = routingTimeInterval;
                tv.tv_usec = 0;
                diffInterval = 0;
            }
            //If the time interval is not completed , compute the
            //remaining time and set it to select()
            else
            {
                tv.tv_sec = routingTimeInterval - diffInterval/1000;
                tv.tv_usec = (diffInterval%1000)*1000;
            }
            prevTime = currentTime;
        }
    }
}

/*
Index to server Id mapping , used to find the index using server id
*/
int Routing::GetIndex(int serverID)
{
    for(int i = 0 ; i < numOfServers ; i++)
    {
        if(serverID == _indexToServerID[i])
        {
            return i;
        }
    }
    return 0;
}

/*
Add the neighbor details after reading from topology file
*/
void Routing::AddNeighborDetails(int s ,int cost)
{
    nList[GetIndex(s)].cost = cost;
    nList[GetIndex(s)].serverID = s;
}


/*
Add server details after reading from topology file
index id to server id mapping is also done here
*/
void Routing::AddServerDetails(int serverID,char *IP, int portNum , int ind)
{
    sList[ind].portNumber = portNum;
    sList[ind].serverID = serverID;
    strcpy(sList[ind].IP,IP);
    _indexToServerID[ind] = serverID;

}


/*
Read the given topology file and fill the server and neighbor details in the
respective structures
*/
void Routing::ReadTopologyFile(char *fileName , int time)
{
    //Read the input file
    std::ifstream infile(fileName);
    std::string line;

    routingTimeInterval = time;

    if(std::getline(infile, line))
    {
        std::istringstream iss(line);
        if (!(iss >>numOfServers))
        {
            cout<<"Error in reading number of servers\n";
            return;
        }

    }

    if(std::getline(infile, line))
    {
        std::istringstream iss(line);
        if (!(iss >>numOfNeighbors))
        {
            cout<<"Error in reading number of neighbors\n";
            return;
        }

    }
 //   cout<<numOfServers<<"\n";
 //   cout<<numOfNeighbors<<"\n";

    for(int i = 0 ; i < numOfServers ; i++)
    {
        int portNumber;
        int serverID;
        std::string IP;

        if(std::getline(infile, line))
        {
            std::istringstream iss(line);
            if (!(iss >>serverID>>IP>>portNumber))
            {
                cout<<"Error in reading server ID and port number\n";
                return;
            }

        }
   //     cout<<serverID<<" "<<IP<<" "<<portNumber<<endl;
        AddServerDetails(serverID,(char*)IP.c_str(),portNumber,i);
    }

    selfServerID = GetServerID();
    for(int i = 0 ; i < numOfNeighbors ; i++)
    {
        int serverID1;
        int serverID2;
        int cost;

        if(std::getline(infile, line))
        {
            std::istringstream iss(line);
            if (!(iss >>serverID1>>serverID2>>cost))
            {
                cout<<"Error in reading server ID's and cost\n";
                return;
            }

        }

  //      cout<<serverID1<<" "<<serverID2<<" "<<cost<<endl;
        if(serverID1 == selfServerID)
        {
            AddNeighborDetails(serverID2,cost);
        }
        else
        {
            AddNeighborDetails(serverID1,cost);
        }

    }

    DistanceVector::GetInstance()->DistanceVectorInitialization();
    DistanceVector::GetInstance()->ComputeDistanceVector();
}

int main(int argc , char *argv[])
{

    if (argc != 5)
    {
        cout<<"Error!!! expecting 5 command line arguments\n";
        return -1;
    }

    char fileName[128] = {0,};
    int timeInterval = 0;
    strcpy(fileName,argv[2]);


    if(HelperFunctions::CheckStringToIntValidity(argv[4]) == true)
    {
        timeInterval = atoi(argv[4]);
        Routing::GetInstance()->ReadTopologyFile(fileName,timeInterval);
        Routing::GetInstance()->Manager();
    }
    else
    {
          cout<<"Invalid time interval Number\n";
    }

    return 0;

}

Routing *Routing::server_instance = 0;
