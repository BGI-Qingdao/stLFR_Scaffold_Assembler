
#ifndef __DESIGN_PATTERN_FILTER_H__
#define __DESIGN_PATTERN_FILTER_H__

#include <map>
#include <cassert>

#include "design_pattern/Publish_Subscribe.h"
namespace BGIQD {
    namespace DesignPattern {

        template<class K , class P>
            struct Filter
            {
                typedef K Key ;

                typedef P Publisher;

                typedef typename Publisher::Subscriber Subscriber;

                typedef typename Publisher::Message Message;

                std::map<K , P >  _filter_map;

                void watch(const Key & k ,Subscriber * ptr)
                {
                    assert(ptr != NULL  );
                    _filter_map[k].add(ptr);
                }

                void notify_with_msg( const Key & k , const Message & msg)
                {
                    auto  itr = _filter_map.find(k) ;
                    if( itr != _filter_map.end() )
                        itr->second.notify_with_msg(msg);
                }
            };

    }
}
#endif