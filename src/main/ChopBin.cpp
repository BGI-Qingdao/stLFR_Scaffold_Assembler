/**********************************************************
 *
 * @Brief :
 *      Chop contig into bins, there strategy are supported:
 *        + chop bin with same size back to back
 *        + chop only two bin : head and tail
 *        + treat entire contig with 1 bin.
 *
 * *******************************************************/

#include "utils/interval/Interval.h"
#include "utils/files/file_reader.h"
#include "utils/files/file_writer.h"
#include "utils/args/argsparser.h"
#include "utils/log/log.h"
#include "utils/string/stringtools.h"
#include "utils/misc/Error.h"

#include "utils/misc/fileName.h"
#include "utils/misc/contigIndex.h"
#include "utils/misc/contigIndex.h"

#include "stLFR/ContigBinBarcode.h"

#include <vector>
#include <map>
#include <set>
#include <tuple>
#include <sstream>
#include <random>

//
// Struct to wrap all global variables and functions
//
struct AppConfig
{
    //                    contigId        info for one contig
    typedef std::map<unsigned int, BGIQD::stLFR::ContigBarcodeInfo>  BarcodeOnContig;
    typedef BGIQD::INTERVAL::Interval<int,
                BGIQD::INTERVAL::Left_Close_Right_Close > BinInterval;

    BGIQD::LOG::logger log;

    int bin_size ;

    int del_at_tail;

    float bin_factor ;


    enum WorkingMode
    {
        Unknow = 0 ,
        EqualBin = 1 ,
        Head_Tail = 2 ,
        OneBin = 3 
    };

    WorkingMode work_mode ;

    bool head_tail_only;

    BarcodeOnContig boc;

    std::map<unsigned int ,BGIQD::SOAP2::ContigIndex> seeds;

    BGIQD::MISC::FileNames fName;

    BGIQD::stLFR::BarcodeOnBinArray b2b_array;

    float sample_fac ;

    std::tuple<bool , int > IsBarcodeInBin( const BinInterval & bin , int barcode_start_pos )
    {
        bool in = bin.IsContain(barcode_start_pos);
        int len = in ? 1 : 0 ;
        return std::make_tuple( in , len );
    }

    std::map<int ,BinInterval> MakeBin(int contig_len)
    {
        std::map<int , BinInterval > ret ;
        int contig_used_len = contig_len - del_at_tail;

        if( work_mode == WorkingMode::OneBin)
        {
            ret[0] = BinInterval(-100,contig_used_len+100);
            return  ret ;
        }
        else if ( work_mode == WorkingMode::Head_Tail)
        {
            int half = contig_used_len / 2 ;
            int end1 = half -1 ;
            int start2 = half ;
            if ( end1 > max_bin_size )
            {
                end1 = max_bin_size ;
                start2 = contig_used_len - max_bin_size +1 ;
            }
            if( end1 > bin_size )
                end1 = bin_size ;
            if( start2 < ( contig_used_len - bin_size +1) )
                start2 = contig_used_len - bin_size +1 ;

            ret[0] = BinInterval( 1 , end1 );
            ret[1] = BinInterval( start2 , contig_used_len);
            return ret ;
        }
        else if ( work_mode != WorkingMode::EqualBin )
        {
            assert(0);
        }
        else
        {
            ;
        }

        int start = 1 ;
        int end = contig_used_len ;
        int bin_num = contig_used_len / bin_size + ( (float ( contig_used_len % bin_size ) /(float) bin_size) >= bin_factor? 1 : 0 );
        if ( bin_num == 1 )
        {
            start += (end -start- bin_size) / 2;
            ret[0] = BinInterval(start , start+bin_size -1 );
        }
        else 
        {
            assert( bin_num >= 2 );
            for( int i = 0  ; i< bin_num ; i++ )
            {
                assert( start <= end );
                if( i % 2 == 0 )
                {
                    // Make sure we do not chop bin from very middle area .
                    if ( start > max_bin_size )
                    {
                        break ;
                    }
                    ret[i/2] = 
                        BinInterval(start , start+bin_size -1 );
                    start += bin_size ;
                }
                else
                {
                    ret[bin_num-(i/2)-1] = 
                        BinInterval(end -bin_size +1 , end );
                    end -= bin_size ;
                }
            }
        }
        assert( bin_num > 0 );
        return ret ;
    };

    void LoadSeeds()
    {
        BGIQD::LOG::timer t(log,"LoadSeeds");
        auto in = BGIQD::FILES::FileReaderFactory::
            GenerateReaderFromFileName(fName.seeds(middle_name)) ;
        if( in == NULL )
            FATAL( "open .seeds file to read failed !!! " );

        std::string line ;
        while( in && !std::getline(*in, line).eof() )
        {
            BGIQD::SOAP2::ContigIndex tmp;
            tmp.InitFromString(line);
            seeds[tmp.contig] = tmp;
            boc[tmp.contig].contig_id= tmp.contig;
        }
        delete in ;
    }

    bool ValidBarcode(unsigned int b) const {
        if(sample_fac >= 1.0f ) return true ;
        return valid_barcodes.find(b) != valid_barcodes.end();
    }

    std::set<unsigned int> valid_barcodes ;
    void SampleBarcodes(){
        if( sample_fac >= 1.0f ) return ;
        std::set<unsigned int > all_barcodes;
        // Load barcode list
        auto in = BGIQD::FILES::FileReaderFactory::
            GenerateReaderFromFileName(fName.barcodeList()) ;
        if( in == NULL )
            FATAL( "open xxx.barcodeList file to read failed !!! " );

        std::string line ;
        int total = 0 ; int sampled=0;
        srand(0);
        while( in && !std::getline(*in, line).eof() )
        {
            std::string name;
            unsigned int id;
            std::istringstream ist(line);
            ist>>name>>id;
            if( id == 0 ) continue ;
            total ++ ;
            float r = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
            if( r < sample_fac ) {
                valid_barcodes.insert(id);
                sampled ++ ;
            }
        }
        std::cerr<<" Use minhash method , sample "<<sampled<<" barcode from total "<<total<<'\n';
        delete in;
        in = NULL;
    }

    void Init(const std::string & prefix
            ,const  int bin 
            , float bin_f)
    {
        bin_size = bin;
        del_at_tail = 0;
        bin_factor = bin_f;
        log.Init("ChopBin");
        fName.Init(prefix);
        b2b_array.Init(1024);
        log<<BGIQD::LOG::lstart()<<"Init finsish ..."<<BGIQD::LOG::lend();

        assert( int(work_mode) > 0 );
        assert( int(work_mode) < 4 );
    }

    void LoadBarcodeOnContig()
    {
        auto in = BGIQD::FILES::FileReaderFactory::GenerateReaderFromFileName(fName.BarcodeOnContig());
        if(! in )
            FATAL( "failed to open xxx.barcodeOnContig to read !");
        BGIQD::LOG::timer t(log,"parse all input");
        std::string line;
        while(!std::getline(*in,line).eof())
        {
            BGIQD::stLFR::ContigBarcodeInfo tmp ;
            tmp.InitFromString(line);
            if( boc.find( tmp.contig_id) == boc.end() )
            {
                continue;
            }
            boc[tmp.contig_id] = tmp ;
        }
        delete in;
    }


    void ChopBin()
    {
        for(const auto & contig :boc )
        {
            unsigned int contigId = contig.first;
            int len = seeds[contigId].length ;
            auto bin_intervals = MakeBin(len);
            std::map<int,BGIQD::stLFR::BarcodeOnBin> b2b;
            for( const auto & posData : contig.second.barcodesOnPos )
            {
                int pos = posData.first ;
                for( const auto & vv : posData.second )
                {
                    if( ! ValidBarcode(vv) ) continue ;
                    for( const auto & pair : bin_intervals )
                    {
                        bool in ; int in_len ;
                        std::tie( in , in_len ) = IsBarcodeInBin( pair.second , pos ) ;
                        //if(pair.second.IsContain(pos))
                        if( in && in_len > 0 )
                        {
                            int binId = pair.first ;
                            auto & d = b2b[binId];
                            d.start = pair.second.min ;
                            d.end = pair.second.max ;
                            d.contigId = contigId;
                            d.binId = binId ;
                            d.collections.IncreaseElement(vv,in_len);
                        }
                    }
                }
            }

            for( const auto & i : b2b )
            {
                b2b_array.push_back(i.second);
            }
        }
    }

    void PrintBinInfo()
    {
        BGIQD::stLFR::PrintBarcodeOnBinArray(fName.BarcodeOnBin(middle_name),b2b_array);
    }

    int max_bin_size ;
    std::string middle_name ;
}config;

int main(int argc , char ** argv)
{
    START_PARSE_ARGS
    DEFINE_ARG_REQUIRED(int , bin_size, "bin size . must be smaller than seed min length");
    DEFINE_ARG_REQUIRED(std::string ,prefix, "prefix . Input xxx.seeds && xxx.barcodeOnContig ; Output xxx.barcodeOnBin");
    //DEFINE_ARG_OPTIONAL(bool ,p_b2c , "print barcode on contig", "0");
    DEFINE_ARG_OPTIONAL(int , work_mode, " the work_mode for chopbin : \n\
                            1 for chop bin with equal bin size \n\
                            2 for chop bin only at contig head and tail \n\
                            3 for chop 1 bin for a contig ", "1");

    DEFINE_ARG_OPTIONAL(float ,bin_factor , "factor of smallest bin in the middle", "0.5");
    DEFINE_ARG_OPTIONAL(int,  max_bin_size , "max bin area from head & tail " ,"15000");
    DEFINE_ARG_OPTIONAL(std::string,  middle_name, "the middle name of output suffix " ,"");
    DEFINE_ARG_OPTIONAL(float ,sample_factor, "random sample barcodes to reduce cluster time. [0.1,1]", "1");
    END_PARSE_ARGS

    config.work_mode = static_cast<AppConfig::WorkingMode>(work_mode.to_int());
    config.max_bin_size = max_bin_size.to_int();
    config.sample_fac = sample_factor.to_float() ;
    config.middle_name = middle_name.to_string() ;
    config.Init( prefix.to_string() , bin_size.to_int() , bin_factor.to_float());

    BGIQD::LOG::timer t(config.log,"ChopBin");

    config.LoadSeeds();

    if( config.sample_fac < 0.1 )  config.sample_fac =0.1;
    if( config.sample_fac > 1 )  config.sample_fac = 1;
    if( config.sample_fac < 1 )
        config.SampleBarcodes();

    config.LoadBarcodeOnContig();

    config.ChopBin();

    config.PrintBinInfo();

    return 0;
}
