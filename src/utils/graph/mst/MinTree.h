#ifndef __ALGORITHM_GRAPH_MINTREE_H__
#define __ALGORITHM_GRAPH_MINTREE_H__

#include "utils/graph/mst/fib_heap.h"
#include <map>
#include <cassert>
/**********************************************************
 *
 * @Brief :
 *  The implement of Prim's minimum spanning tree algorithm which strict follow the book "Introduction to Algorithms".
 *
 * *******************************************************/
namespace BGIQD {
    namespace GRAPH {

        // User implement this to get weight:
        template<class TNode,class TEdge, class Value>
            struct NodeAttr
            {
                Value GetValue( const TEdge & ) const {}
            };


        template<class TListGraph 
            , class TValue 
            , class TNodeAttr =
            NodeAttr<typename TListGraph::Node 
            ,typename TListGraph::Edge
            , TValue> >
            struct MinTreeHelper
            {
                typedef TListGraph  TheGraph;
                typedef TNodeAttr   TheAttr;
                typedef TValue      Value;

                typedef typename TListGraph::Node Node;
                typedef typename TListGraph::Edge Edge;
                typedef typename Edge::EdgeEdgeId EdgeId;
                typedef typename Edge::EdgeNodeId NodeId;

                typedef typename BGIQD::FIBHEAP::Node<TValue, NodeId> fibNode;

                struct MTSearchNode
                {
                    fibNode  fib_node ;
                    EdgeId   smallest_edge_id ;
                };

                typedef typename BGIQD::FIBHEAP::FibHeap<fibNode> fibHeap ;

                TheGraph MinTree(const TheGraph & base , const TheAttr & attr )
                {
                    std::map<NodeId, MTSearchNode>  fib_nodes ;
                    TheGraph ret ;
                    fibHeap heap ;

                    auto update_node_into_heap = [&]( const Node & n, const Edge & edge , Value curr_value ) -> void
                    {
                        auto itr = fib_nodes.find( n.id ) ;
                        if( itr != fib_nodes.end() )
                        {
                            if( curr_value < itr->second.fib_node.key )
                            {
                                heap.DecreaseKey(itr->second.fib_node , curr_value);
                                itr->second.smallest_edge_id = edge.id ;
                            }
                            return ;
                        }

                        if( n.EdgeNum() ==0 )
                        {
                            assert(0);
                            return ;
                        }

                        auto & node = fib_nodes[n.id];
                        node.fib_node.Init();
                        node.fib_node.value = n.id ;
                        node.fib_node.key = curr_value;
                        node.smallest_edge_id = edge.id ;
                        heap.Insert(node.fib_node);
                    };

                    auto add_node_into_mintree = [&](const Node & node ) -> void 
                    {
                        ret.AddNode(node);
                        ret.GetNode(node.id).CleanEdges();
                        if( node.EdgeNum() < 1 )
                        {
                            return ;
                        }
                        typename Node::NodeEdgeIdIterator begin,end;
                        std::tie(begin,end) = node.GetEdges();
                        for( auto x = begin ; x!=end; x++ )
                        {
                            const auto & edge = base.GetEdge(*x);
                            auto oppo = edge.OppoNode(node.id);
                            if( ret.HasNode(oppo) )
                                continue ;

                            update_node_into_heap(base.GetNode(oppo) , edge ,attr.GetValue(edge));
                        }
                    };

                    for( const auto & base_node_pair : base.nodes )
                    {
                        const auto & base_node = base_node_pair.second ;
                        if( ! ret.HasNode(base_node.id) )
                        {
                            add_node_into_mintree(base_node);
                            while(! heap.Empty() )
                            {
                                auto & node = heap.ExtractMin();
                                NodeId curr_node = node.value ;
                                auto & base_mt_node = fib_nodes.at(curr_node);
                                const Edge & edge = base.GetEdge(base_mt_node.smallest_edge_id) ;
                                add_node_into_mintree(base.GetNode(curr_node));
                                ret.AddEdge(edge);
                            }
                        }
                    }
                    return ret ;
                }
            };
    }
}

#endif //__ALGORITHM_GRAPH_MINTREE_H__
