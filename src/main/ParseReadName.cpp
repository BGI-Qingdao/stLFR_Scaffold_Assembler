#include "common/args/argsparser.h"
#include "common/files/file_reader.h"
#include "common/files/file_writer.h"
#include "common/error/Error.h"
#include "common/log/log.h"
#include "common/log/logfilter.h"
#include "common/string/stringtools.h"
#include "common/freq/freq.h"

#include "soap2/fileName.h"
#include "stLFR/StringIdCache.h"
#include "biocommon/fastq/fastq.h"

struct AppConfig
{
    typedef BGIQD::FASTQ::stLFRHeader::ReadType Type ;
    typedef BGIQD::FASTQ::Fastq<BGIQD::FASTQ::stLFRHeader> Fastq;
    typedef BGIQD::FASTQ::FastqReader<Fastq> Reader;

    BGIQD::LOG::logger loger;

    BGIQD::stLFR::StringIdCache barcode_list;

    BGIQD::stLFR::StringIdCache unknow_barcode_list;

    BGIQD::FREQ::Freq<std::string>  barcode_freq;

    int min_pair_threshold ;

    int max_pair_threshold ;

    std::string read1;

    BGIQD::SOAP2::FileNames fNames;

    void Init(const std::string & in , const std::string & prefix)
    {
        read1 = in ;
        fNames.Init(prefix);
        BGIQD::LOG::logfilter::singleton().get("ParseReadName",BGIQD::LOG::DEBUG,loger);
    }

    void ParseRead1()
    {
        long index = 1 ;
        auto out = BGIQD::FILES::
            FileWriterFactory::GenerateWriterFromFileName
            (fNames.readNameList());

        BGIQD::LOG::timer timer(loger,"ParseRead1");

        if( out == NULL )
            FATAL( " failed to open xxx.readNameList for write !!! ");

        auto in = BGIQD::FILES::FileReaderFactory::GenerateReaderFromFileName(read1);
        if ( in == NULL )
            FATAL( " failed to open read1 for read !!! ");
        Fastq data;
        while( Reader::LoadNextFastq(*in , data))
        {
            (*out)<<data.head.readName<<'\t'<<index<<'\n';
            index ++ ;
            if( index >= 1000000 && index % 1000000 == 1 )
            {
                loger<<BGIQD::LOG::lstart()<<"process "
                    <<index<<" read... "<<BGIQD::LOG::lend();
            }

            if( data.head.type == Type::readName_barcodeStr_index_barcodeNum )
            {
                barcode_list.preload = true ;
                barcode_list.data.AssignTag(
                        data.head.barcode_str,
                        data.head.barcode_num);
            }
            else
            {
                unknow_barcode_list.preload = true ;
                unknow_barcode_list.Id(data.head.barcode_str);
            }
            barcode_freq.Touch(data.head.barcode_str);
        }
        delete in ;
        delete out ;
    }

    void PrintBarcodeList()
    {
        if( barcode_list.preload == unknow_barcode_list.preload )
        {
            loger<<BGIQD::LOG::lstart()
                <<"ERROR :  some has barcode "<<barcode_list.preload
                <<" some has no barcode "<<unknow_barcode_list.preload
                <<BGIQD::LOG::lend();
        }
        BGIQD::LOG::timer timer(loger,"PrintBarcodeList");
        if( barcode_list.preload )
        {
            loger<<BGIQD::LOG::lstart()<<"has barcode num in read1"
                <<BGIQD::LOG::lend();
            barcode_list.Print(fNames.barcodeList());
        }
        else
        {
            loger<<BGIQD::LOG::lstart()<<"no barcode num in read1"
                <<BGIQD::LOG::lend();
            unknow_barcode_list.Print(fNames.barcodeList());
        }
    }

    void MaskTooLowBarcode()
    {
        for( const auto & pair : barcode_freq.data )
        {
            long num = pair.second ;
            const std::string & barcode_str = pair.first ;
            if( num < min_pair_threshold )
                barcode_list.data.AssignTag(barcode_str,0);
        }
    }

    void MaskTooHighBarcode()
    {
        for( const auto & pair : barcode_freq.data )
        {
            long num = pair.second ;
            const std::string & barcode_str = pair.first ;
            if( num > max_pair_threshold )
                barcode_list.data.AssignTag(barcode_str,0);
        }
    }

    void PrintBarcodeFreq()
    {
        auto bfreq = BGIQD::FILES::FileWriterFactory
            ::GenerateWriterFromFileName(fNames.barcodeFreq());
        if( bfreq == NULL )
            FATAL(" failed to open xxx.barcodeFreq to write !!! ");

        (*bfreq)<<barcode_freq.ToString();
        delete bfreq ;
    }

}config;

int main(int argc , char **argv )
{
    START_PARSE_ARGS
        DEFINE_ARG_REQUIRED(std::string , read1 , "read 1 for parse ");
        DEFINE_ARG_REQUIRED(std::string , prefix , "prefix . Output xxx.barcodeList xxx.barcodeFreq xxx.readNameList");
        DEFINE_ARG_OPTIONAL(int , min_pair_threshold , " the min_pair_threshold " , "10");
        DEFINE_ARG_OPTIONAL(int , max_pair_threshold , " the max_pair_threshold " , "500");
    END_PARSE_ARGS

    config.max_pair_threshold = max_pair_threshold.to_int() ;
    config.min_pair_threshold = min_pair_threshold.to_int() ;

    config.Init(read1.to_string() , prefix.to_string());
    BGIQD::LOG::timer timer(config.loger,"ParseReadName");

    config.ParseRead1() ;
    config.MaskTooLowBarcode() ;
    config.MaskTooHighBarcode() ;

    if( config.barcode_list.preload )
    {
        config.PrintBarcodeList();
        config.PrintBarcodeFreq();
    }
}
