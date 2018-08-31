#include <cassert>
#include <algorithm>
#include <sstream>

#include "common/files/file_reader.h"
#include "common/error/Error.h"
#include "stLFR/CBB.h"
#include "stLFR/barcodeOnContig.h"
namespace BGIQD {
    namespace stLFR {

        // ---------------- GraphEA_withBarcode----------------

        int GraphEA_withBarcode::Union(const std::map<int,int> & m1 
                    , const std::map<int,int> & m2 
                    , std::map<int, int> & um )
        {

            int ret = 0 ;
            for(const auto pair : m2)
            {
                if( m1.find( pair.first ) != m1.end() )
                {
                    int min = pair.second < m1.at(pair.first)
                        ? pair.second
                        : m1.at(pair.first) ;

                    ret += min ;
                    um[pair.first] = min ;
                }
                ret += pair.second;
            }
            return ret;
        }

        // ---------------- Path -------------------------

        void P2PGraph::Path::MergeCircle()
        {
            if( ! circle.Is_set() )
                return ;
            unsigned int circle_root = circle.cpath[0].id ;
            size_t i = 0;
            for( i = 0 ; i < paths.size() ; i++ )
            {
                if( paths[i].id == circle_root ) 
                    break;
            }
            paths.insert(paths.begin() + i ,circle.cpath.begin() , circle.cpath.end());

        }

        // ---------------- P2PGraph -------------------------

        void P2PGraph::Init( unsigned int from , unsigned int to)
        {
            target = to ;
            root= from;
            final_circled = 0 ; 
            InitEdge(to);
            InitEdge(from);
            auto & from_n = base_graph->graph_ea.edge_array[from];
            auto & to_n = base_graph->graph_ea.edge_array[to];
            unsigned int key_from = from_n.IsKey() ? from_n.id : from_n.bal_id ;
            unsigned int key_to = to_n.IsKey() ? to_n.id : to_n.bal_id ;
            GraphEA_withBarcode::Union(base_graph->barcode_on_contig[key_from] ,base_graph->barcode_on_contig[key_to],root_target_union);
        }

        void P2PGraph::InitEdge( unsigned int id)
        {
            if(sub_graph.find(id) == sub_graph.end() )
            {
                auto & node = base_graph->graph_ea.edge_array[id];
                sub_graph[id].id = id;
                sub_graph[id].length =  node.length;
                sub_graph[id].cov = node.cov;
                if( id != root && id != target )
                {
                    BarcodeOnContig tmp;
                    if( base_graph->barcode_on_contig.find(id)  != base_graph->barcode_on_contig.end() )
                    {
                        auto & id_n = base_graph->graph_ea.edge_array[id];
                        unsigned int key_id = id_n.IsKey() ? id_n.id : id_n.bal_id ;
                        sub_graph[id].barcode_cov = GraphEA_withBarcode::Union(base_graph->barcode_on_contig[key_id],root_target_union,tmp);
                    }
                }
            }
        }

        void P2PGraph::AddFromTo( unsigned int from , unsigned int to )
        {
            InitEdge( from );
            InitEdge( to );
            sub_graph.at(from).tos.insert(to);
        }

        void P2PGraph::RemovePalindromeLink()
        {
            for( auto & i : sub_graph )
            {
                auto & node = base_graph->graph_ea.edge_array[i.second.id];
                if( node.IsPalindrome() )
                {
                    std::set<unsigned int> to_del ;
                    for( auto to : i.second.tos)
                    {
                        auto & a_to = sub_graph.at(to) ;
                        if( a_to.tos.find( i.second.id ) != a_to.tos.end() )
                        {
                            to_del.insert(to);
                        }
                    }
                    for( auto to : to_del )
                    {
                        i.second.tos.erase(to);
                    }
                }
            }
        }

        bool P2PGraph::CheckPalindrome() const
        {
            for( const auto & i : sub_graph )
            {
                const auto & node = base_graph->graph_ea.edge_array[i.second.id];
                if( node.IsPalindrome() )
                {
                    return true ;
                }
            }
            return false ;
        }

        void P2PGraph::findAllPath()
        {
            Path p;
            p.Init();
            p.K = K ;
            path_num = 0 ;
            auto & node = sub_graph[root];
            for( const auto &i : node.tos)
            {
                Circle c;
                c.Clean() ;
                findAllPath(i, p, c);
                if( path_num < 0 || path_num > 100 )
                    return ;
            }
            if( path_num < 0 )
            {
                allPaths.clear();
            }
            else
            {
                path_num = allPaths.size();
            }
        }

        void P2PGraph::findAllPath( unsigned int id  ,Path p , Circle & circle)
        {
            if( p.junction  > 10 )
                return ;
            if( deal_circle == P2PGraph::CircleStrategy::FillCircle )
            {
                if( p.IsPathInCircle(circle ) )
                {
                    p.AddCircle(circle);
                }
                else
                {
                    circle.Clean();
                }
            }
            unsigned int curr = id;
            while ( sub_graph[curr].tos.size() == 1 && curr != target )
            {
                if( ! p.AddEdge(sub_graph[curr]) )
                {
                    if( deal_circle == P2PGraph::CircleStrategy::FillCircle )
                    {
                        if( ! circle.Is_set() )
                        {
                            circle.SetCircle( p.paths , curr, ecov );
                        }
                        else
                            path_num = -2 ;
                    }
                    else if ( deal_circle == P2PGraph::CircleStrategy::IgnoreCircle )
                        return ;
                    else  if ( deal_circle == P2PGraph::CircleStrategy::GiveUp )
                        path_num = -1 ;
                    else 
                        assert(0);

                    return ;
                }
                curr = * sub_graph[curr].tos.begin();
            }
            if( path_num < 0 )
                return ;
            if ( curr == target )
            {
                allPaths.push_back(p);
                path_num ++ ;
            }
            else if( sub_graph[curr].tos.empty() )
            {
                return ;
            }
            else {
                p.junction ++ ;
                assert( sub_graph[curr].tos.size() >1 );
                if( ! p.AddEdge(sub_graph[curr]) )
                {
                    if( deal_circle == P2PGraph::CircleStrategy::FillCircle )
                    {
                        if( ! circle.Is_set() )
                        {
                            circle.SetCircle( p.paths , curr , ecov );
                        }
                        else 
                            path_num = -2 ;
                    }
                    else if ( deal_circle == P2PGraph::CircleStrategy::IgnoreCircle )
                        return ;
                    else  if ( deal_circle == P2PGraph::CircleStrategy::GiveUp )
                        path_num = -1 ;
                    else
                        assert(0);

                    return ;
                }
                for( const auto & i : sub_graph[curr].tos )
                {
                    findAllPath(i ,p,circle);
                    if( path_num < 0 || path_num > 100 )
                        return ;
                }
            }
        }

        int P2PGraph::ShortestPath()
        {
            findAllPath();
            return 0;
        }
        void P2PGraph::ScoreAllPath()
        {
            if( path_num > 1 && path_num < 100  )
            {
                for(auto & i : allPaths )
                {
                    if( deal_circle )
                        i.MergeCircle();
                    i.CalcCov();
                }
                std::sort( allPaths.rbegin() ,allPaths.rend() );
            }
            else if ( path_num == 1 )
            {
                if( deal_circle )
                    allPaths[0].MergeCircle();
            }
        }

        void P2PGraph::CleanAndSavePath()
        {
            if( path_num > 0 && path_num < 100)
            {
                final_path.clear();
                auto & correct = * allPaths.begin();
                for( const  auto & i : correct.paths )
                {
                    final_path.push_back(i.id) ;
                }
                final_circled = correct.circle.circle_run ;
                allPaths.clear();
            }
            else
            {
                allPaths.clear();
                if( path_num > 100 )
                    path_num  = - path_num ;
            }
        }


        void P2PGraph::GeneratePath()
        {
            findAllPath();
            ScoreAllPath();
            CleanAndSavePath();
        }

        ////        void P2PGraph::ConstructSubP2pGraph()
        //       {
        //           ConstructSubP2pGraph(root_graph);
        //       }

        void GraphEA_withBarcode::LoadBarcodeOnConfig(const std::string & file)
        {
            auto in = BGIQD::FILES::FileReaderFactory::GenerateReaderFromFileName(file);
            if( in == NULL )
                FATAL(" open xxx.barcodeOnContig to read failed !!! ");
            std::string line;
            while(!std::getline(*in,line).eof())
            {
                BGIQD::stLFR::ContigBarcodeInfo tmp ;
                tmp.InitFromString(line);
                unsigned int contigId = tmp.contig_id ;
                for(const auto pair : tmp.barcodesOnPos)
                {
                    for( unsigned int barcode: pair.second)
                    {
                        if( barcode_on_contig[contigId].find(barcode)
                                == barcode_on_contig[contigId].end())
                            barcode_on_contig[contigId][barcode] =1 ;
                        else
                            barcode_on_contig[contigId][barcode] ++ ;
                    }
                }
            }
            delete in;
        }

    }// namespace stLFR
}// namespace BGIQD
