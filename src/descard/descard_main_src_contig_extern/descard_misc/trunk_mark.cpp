#include "common/args/argsparser.h"
#include "common/files/file_reader.h"
#include "common/files/file_writer.h"
#include "common/string/stringtools.h"
#include "common/freq/freq.h"

#include "algorithm/interval/Interval.h"

#include <iostream>
#include <set>

typedef BGIQD::INTERVAL::Interval<int,BGIQD::INTERVAL::IntervalType::Left_Close_Right_Close> ContigArea;
struct GlobalConfig 
{
    std::string seedLinear ;
    std::string trunkLinear;
    std::string outPrefix;
    int bin;
    std::map<unsigned int , std::set<int> > seedPos;
    std::map<unsigned int , std::set<ContigArea> > seedArea;
    std::map<unsigned int , int > seedLens;

    std::set<unsigned int> repeats;
    std::vector<ContigArea> scaffold_area;
    std::set<unsigned int> seeds ;

    std::map<unsigned int , int> gaps;

    void LoadSeedLinear()
    {
        auto sf = BGIQD::FILES::FileReaderFactory::GenerateReaderFromFileName(seedLinear);
        std::string line;
        int line_index = 1 ;
        while(!std::getline(*sf,line).eof())
        {
            auto items = BGIQD::STRING::split(line,"\t");
            seedPos[std::stoul(items[0])].insert( line_index) ;
            seedLens[std::stoul(items[0])] = std::stoul(items[1]);
            seedArea[std::stoul(items[0])].insert(ContigArea(std::stoi(items[2]),std::stoi(items[3])));
            line_index ++ ;
        }
        delete sf ;
    }

    void ParseTrunkLinear()
    {
        auto tf = BGIQD::FILES::FileReaderFactory::GenerateReaderFromFileName(trunkLinear);
        std::ostream * out = NULL ;
        int trunk_index = 1 ;

        std::string line ;
        ContigArea a_scaf(-1,-1);
        int p_s = -1 , p_e = -1 ;
        unsigned int prev; 
        while(!std::getline(*tf,line).eof())
        {
            if(line[0] == '-')
            {
                p_s = - 1 ;
                p_e = -1 ;
                if( out != NULL )
                    delete out ;
                out = NULL ;
                std::string of = outPrefix + std::to_string(trunk_index);
                trunk_index ++ ;
                out = BGIQD::FILES::FileWriterFactory::GenerateWriterFromFileName(of);
                if( a_scaf.min != -1 && a_scaf.max != -1 )
                { 
                    if( a_scaf.min > a_scaf.max )
                        std::swap( a_scaf.min , a_scaf.max );
                    scaffold_area.push_back(a_scaf);
                }
                a_scaf = ContigArea(-1,-1);
                continue;
            }
            unsigned int contig = std::stoul(line);
            (*out)<<contig;

            ContigArea area;
            if ( p_s != -1 && seedPos[contig].size() > 1)
            {
                int ps = -1 ;
                for( const auto & a : seedArea[contig] )
                {
                    if(std::abs( a.min - ps ) > ps)
                    {
                        ps = std::abs( a.min - ps );
                        area = a ;
                    }
                }
            }
            else
            {
                area = *(seedArea[contig].begin());
            }

            seeds.insert( contig );
            if( a_scaf.min == -1 )
            {
                a_scaf.min = area.min ;
                a_scaf.max = area.max ;
            }
            else
                a_scaf.max = area.min ;
            if( p_s != -1 && p_e != -1 )
            {

                if(area.min + 63 > p_e )
                    gaps[prev] = area.min - p_e ;
                else
                    gaps[prev] = p_s - area.max ;
            }
            prev = contig ;
            p_s = area.min ;
            p_e = area.max ;
            for( auto x : seedPos[contig])
            {
                (*out)<<'\t'<<x;
            }
            (*out)<<'\n';
        }
        if( out != NULL )
            delete out ;
        if( a_scaf.min != -1 && a_scaf.max != -1 )
        { 
            if( a_scaf.min > a_scaf.max )
                std::swap( a_scaf.min , a_scaf.max );
            scaffold_area.push_back(a_scaf);
        }
        delete tf ;
    }

    void Report()
    {

        std::cerr<<"____________________"<<std::endl;
        long total = 0;
        for(const auto & scaff : scaffold_area )
        {
            std::cerr<<scaff.min<<'\t'<<scaff.max<<'\t'<<scaff.Len()<<std::endl;
            total += scaff.Len() ;
        }

        std::cerr<<"____________________"<<std::endl;
        for(int i = 0 ; i <= 60000000 ; i+= bin)
        {
            ContigArea area(i , i+bin );
            int scaff_len = 0 ;
            int contig_len = 0;
            for( const auto & scaff : scaffold_area )
            {
                scaff_len += area.Overlap(scaff).Len();
            }

            for( const auto & s: seeds)
            {
                if( seedArea[s].size() == 1 )
                    contig_len += area.Overlap(*seedArea[s].begin()).Len();
            }
            std::cerr<<i<<'\t'<<scaff_len<<'\t'<<contig_len<<std::endl;
        }
        std::cerr<<"____________________"<<std::endl;
        long over = 0 ;
        for( int i = 0 ; i < (int)scaffold_area.size() ; i ++ )
        {
            for( int j = i + 1 ; j < (int)scaffold_area.size() ; j ++ )
            {
                auto overarea = scaffold_area[i].Overlap(scaffold_area[j]);
                over += overarea.Len() ;
            }
        }
        long contigLen = 0;
        for( const auto & s: seeds)
        {
            if( seedArea[s].size() == 1 )
            {
                contigLen += seedArea[s].begin()->Len();
            }
        }
        std::cerr<<"Total trunk len is "<<total<<std::endl;
        std::cerr<<"Total overlap len is "<<over<<std::endl;
        std::cerr<<"Total contig len is "<<contigLen<<std::endl;
    }

    void Init(const std::string & s , const std::string & t , const std::string & o)
    {
        seedLinear = s ;
        trunkLinear = t ;
        outPrefix = o ;
    }
    void ReportGaps()
    {
        for( const auto & pair : gaps )
        {
            std::cout<<pair.first<<'\t'<<pair.second<<'\n';
        }
    }
}config;

int main(int argc , char **argv)
{
    START_PARSE_ARGS 
    DEFINE_ARG_REQUIRED(std::string , seedLinear , " the seed linear file ");
    DEFINE_ARG_REQUIRED(std::string , trunkLinear , " the trunk linear file ");
    DEFINE_ARG_REQUIRED(std::string , outPrefix, " the out file prefix");
    DEFINE_ARG_OPTIONAL(int ,bin, " the bin size for calc freq","100000");
    DEFINE_ARG_OPTIONAL(bool,gap, " print gaps ","false");
    END_PARSE_ARGS

    config.Init(seedLinear.to_string() , trunkLinear.to_string() , outPrefix.to_string() );
    config.bin = bin.to_int();
    config.LoadSeedLinear() ;
    config.ParseTrunkLinear();
    config.Report() ;
    if( gap.to_bool() )
    {
        config.ReportGaps();
    }
    return 0 ;
}