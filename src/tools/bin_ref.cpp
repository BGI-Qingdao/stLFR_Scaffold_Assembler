#include "common/args/argsparser.h"
#include "common/freq/freq.h"
#include "common/files/file_reader.h"
#include "common/string/stringtools.h"

#include "algorithm/linear_fitting/Minimum_multiplication.h"
#include "algorithm/collection/collection.h"

#include <algorithm>
#include <iostream>
#include <fstream>
#include <map>
#include <cstdlib>
#include <ctime>
#include <unistd.h>
#include <tuple>

struct AppConfig
{
    typedef BGIQD::Collection::Collection<int> Bin;
    typedef BGIQD::FREQ::Freq<float>  Freq;
    Freq f01A , f01B , f01AB , f01ABU;
    Freq f24A , f24B , f24AB , f24ABU;
    //typedef BGIQD::LINEARFITTING::Item<int,float> LSItem;

    //struct
    struct LSItem
    {
        typedef int XType;
        typedef float YType;
        XType x ;
        YType y ;
        float m1;
        float m2;
        float m3;
        float m4;
        float m5;
    };

    std::map<int , std::vector<float>> xys;

    std::vector<LSItem> xydata;

    int limit ;

    int step_max ;

    int start;

    int bin;

    int scs ;

    std::map<int,Bin> bin_data;

    int bin_max ;

    void LoadBarcodeOnRef()
    {
        while( ! std::cin.eof() )
        {
            int bin,num;
            char dot;
            std::cin>>bin>>dot>>num;
            //std::cerr<<"DEBUG : "<<barcode<<":"<<num<<std::endl;
            if ( limit != 0 && bin> start + limit +step_max )
            {
                break;
            }
            if( bin > bin_max )
                bin_max = bin ;
            for(int i = 0 ; i < num ; i++ )
            {
                size_t key, value;
                std::cin>>key>>dot>>value;
                bin_data[bin].IncreaseElement(key, value);
            }
        }
    }

    void CalcAll()
    {
        int max = limit + start + step_max ;
        if ( max > bin_max )
            max = bin_max ;
        for( int i = start ; i <= max - step_max ; i ++ )
        {
            if ( bin_data[i].size() < (size_t)scs )
                continue ;
            for( int j = i + bin ; j <= max && j <= i+step_max ; j++ )
            {
                if ( bin_data[j].size() < (size_t)scs )
                    continue ;
                if( bin_data[i].keysize() > 0 && bin_data[i+1].keysize() > 0)
                {
                    float fac = Bin::Jaccard(bin_data[i],bin_data[j]);
                    if( fac > 0.05 && fac < 0.5)
                        xys[ j - i ].push_back(fac);
                    if( fac < 0.1)
                    {
                        f01A.Touch(bin_data[i].size());
                        f01B.Touch(bin_data[j].size());
                        f01AB.Touch(Bin::Union(bin_data[i],bin_data[j]).size());
                        f01ABU.Touch(Bin::Intersection(bin_data[i],bin_data[j]).size());
                    }
                    if( 0.3 < fac < 0.4)
                    {
                        f24A.Touch(bin_data[i].size());
                        f24B.Touch(bin_data[j].size());
                        f24AB.Touch(Bin::Union(bin_data[i],bin_data[j]).size());
                        f24ABU.Touch(Bin::Intersection(bin_data[i],bin_data[j]).size());
                    }
                }
            }
        }

        for( auto & x : xys )
        {
            float total = 0 ;
            std::sort(x.second.begin() , x.second.end());
            LSItem item;
            for( int i = 0 ; i < (int)x.second.size() ; i ++ )
            {
                total += x.second[i];
                if( i == x.second.size() / 4 )
                    item.m1 = x.second[i];
                if( i == x.second.size() / 2 )
                    item.m2 = x.second[i];
                if( i == x.second.size() / 4 * 3 )
                    item.m3 = x.second[i];
                if( i == int(x.second.size()) -1 )
                    item.m4 = x.second[i];
                if( i == 0 )
                    item.m5 = x.second[i];
            }
            item.x = x.first * 100 ;
            item.y = total / x.second.size();
            xydata.push_back( item);
        }

    }
    void PrintLine()
    {
        if( linear )
        {
            auto ret = BGIQD::LINEARFITTING::lineFit(xydata);
            std::cout<<" a= "<<ret.a<<std::endl;
            std::cout<<" b= "<<ret.b<<std::endl;
            std::cout<<" c= "<<ret.c<<std::endl;
            for( float x = 0.1 ; x < 0.3 ; x += 0.01 )
                std::cout<<x<<" = "<<ret.getX(x)  <<std::endl;
        }
        else
        {
            for( auto & x : xydata)
            {
                std::cout<<x.x <<'\t'
                    <<x.y<<'\t'
                    <<x.m1<<'\t'
                    <<x.m2<<'\t'
                    <<x.m3<<'\t'
                    <<x.m4<<'\t'
                    <<x.m5<<'\t'
                    <<std::endl;
                std::cout<<"F01A"<<f01A.ToString()<<std::endl;
                std::cout<<"F01B"<<f01B.ToString()<<std::endl;
                std::cout<<"F01AB"<<f01AB.ToString()<<std::endl;
                std::cout<<"F01ABU"<<f01ABU.ToString()<<std::endl;

                std::cout<<"F24A"<<f24A.ToString()<<std::endl;
                std::cout<<"F24B"<<f24B.ToString()<<std::endl;
                std::cout<<"F24AB"<<f24AB.ToString()<<std::endl;
                std::cout<<"F24ABU"<<f24ABU.ToString()<<std::endl;
            }
        }
    }
    bool linear;
} config ;

/******************************************************************************
 *
 * Logic.
 *
 *****************************************************************************/

int main(int argc , char ** argv)
{

    START_PARSE_ARGS 
        DEFINE_ARG_REQUIRED(int, limit, " limit ");
        DEFINE_ARG_REQUIRED(int, start, " start ");
    DEFINE_ARG_REQUIRED(int, step, " step ");
    DEFINE_ARG_REQUIRED(int, bin, " bin size /100  ");
    DEFINE_ARG_OPTIONAL(bool, linear, " try linear " , "false");
    DEFINE_ARG_OPTIONAL(int, scs, " smallest Collection size  " , "0");
    END_PARSE_ARGS;

    config.limit = limit.to_int();
    config.step_max = step.to_int();
    config.start = start.to_int() ;
    config.linear = linear.to_bool();
    config.bin = bin.to_int() ;
    config.scs = scs.to_int() ;
    config.LoadBarcodeOnRef();
    config.CalcAll();
    config.PrintLine();

    return 0;
}
