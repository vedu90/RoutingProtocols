#ifndef DISTANCE_VECTOR_H
#define DISTANCE_VECTOR_H
#include "MainRouting.h"

//Routing table is maintained in 1D array of below struct type
//Cost matrix is also maintained in 2D array of below struct type
struct DVTable
{
    int serverID;
    int nextHop;
    int cost;
};

class Routing;
class DistanceVector
{
    int indexToServerID[5];
    static DistanceVector *dv_instance;
    Routing *routingObj;
    DistanceVector()
    {
        memset(selfDV,0,sizeof(selfDV));
        memset(neighborDV,0,sizeof(neighborDV));
    }
    public:

    DVTable selfDV[5];
    DVTable neighborDV[5][5];

    static DistanceVector *GetInstance()
    {
        if (!dv_instance)
          dv_instance = new DistanceVector;
        return dv_instance;
    }
    void DistanceVectorInitialization();
    void ComputeDistanceVector();
    void AddNeighborDVDetails(int serverID1,int serverID2 ,int cost);
 //   void UpdateSelfDV(int s,int cost);
    int GetIndex(int serverID);
};


#endif
