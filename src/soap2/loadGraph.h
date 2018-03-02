#ifndef __SOAP2_LOADGRAPH_H__
#define __SOAP2_LOADGRAPH_H__

#include <string>
#include "soap2/contigGraph.h"
#include <set>
#include <mutex>

namespace BGIQD {
namespace SOAP2 {

struct ContigRoad
{
    bool headin;
    bool tailin;
    int length;
    std::vector<unsigned int> contig;
};

struct GlobalConfig
{
    int K;
    Edge * edge_array;
    Arc * arc_array;
    std::string updateEdge;
    std::string arc;
    std::string cluster;
    unsigned int contigTotalNum;
    unsigned int clusterNum;
    long long arcNum;
    long long connectionNum;

    std::map<unsigned int , std::map<unsigned int,float > > connections;
    std::set<unsigned int> keys;

    // edge id --> key id
    std::map<unsigned int , unsigned int> key_map;
    KeyEdge * key_array;
    std::mutex * key_mutex;


    // super conitgs
    std::mutex contig_mutex;
    std::vector<ContigRoad> contigs ;//std::vector<unsigned int> > contigs;
};

//void loadContigIndex(Edge * array , GlobalConfig & config);
//void loadContig(Edge * array);
//
// 
void loadUpdateEdge( GlobalConfig & config);
void loadArc( GlobalConfig & config);
void loadCluster(GlobalConfig & config);
void buildConnection(GlobalConfig&);
void LinearConnection(GlobalConfig &);
}
}
#endif //__SOAP2_LOADGRAPH_H__
