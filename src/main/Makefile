.PHONY: all clean

CC 		   =	gcc
CXX 	   =	g++

CXXFLAGS   =	-std=c++11\
				-I../\
				-lz\
				-lpthread\

DEUBG_CXX  =	${CXXFLAGS} -g
RELEASE_CXX=	${CXXFLAGS}

source_cpp =	../common/files/file_reader.cpp \
		   		../common/files/file_writer.cpp \
		   		../common/files/gzstream.cpp \
				../common/log/log.cpp\
				../common/log/logfilter.cpp\
				../common/time/timetools.cpp\
				../common/string/stringtools.cpp\
		   		../biocommon/sam_bam/sam_parser.cpp\
		   		../biocommon/pair/pair_sam_parser.cpp\
				../stLFR/barcodeId.cpp\
				../stLFR/readName2Barcode.cpp\
				../stLFR/barcodeOnContig.cpp\
				../stLFR/ContigCluster.cpp\
				../stLFR/LineGroup.cpp\
				../stLFR/CBB.cpp\
				../common/args/argsparser.cpp\
				../soap2/contigGraph.cpp\
				../soap2/contigFasta.cpp\
				../soap2/contigType.cpp\

source_o		= ${source_cpp:%.cpp=%.o}

ChopBin_cpp 	=	ChopBin.cpp
ChopBin_o   =	${ChopBin_cpp:%.cpp=%.o} ${source_cpp:%.cpp=%.o}

BinCluster_cpp =	BinCluster.cpp
BinCluster_o   =	${BinCluster_cpp:%.cpp=%.o} ${source_cpp:%.cpp=%.o}

StaticsticUnique_cpp =	StaticsticUnique.cpp
StaticsticUnique_o   =	${StaticsticUnique_cpp:%.cpp=%.o} ${source_cpp:%.cpp=%.o}

Sam2ReadOnContig_cpp 	= Sam2ReadOnContig.cpp
Sam2ReadOnContig_o 	 	= ${Sam2ReadOnContig_cpp:%.cpp=%.o} ${source_cpp:%.cpp=%.o}

ContigDlink_cpp 		= ContigDlink.cpp
ContigDlink_o			= ${ContigDlink_cpp:%.cpp=%.o} ${source_cpp:%.cpp=%.o}

FillContigRoad_cpp		= FillContigRoad.cpp
FillContigRoad_o 		= ${FillContigRoad_cpp:%.cpp=%.o} ${source_cpp:%.cpp=%.o}

MergeContig_cpp			= MergeContig.cpp
MergeContig_o			= ${MergeContig_cpp:%.cpp=%.o} ${source_cpp:%.cpp=%.o}

LinearCDG_cpp 			= LinearCDG.cpp
LinearCDG_o				= ${LinearCDG_cpp:%.cpp=%.o} ${source_cpp:%.cpp=%.o}

.cpp.o:
	${CXX} ${DEUBG_CXX} -c $< -o $@

jobs 	   =	ChopBin\
				BinCluster \
				StaticsticUnique  \
				Sam2ReadOnContig \
				ContigDlink \
				FillContigRoad \
				MergeContig\
				LinearCDG

jobs_o 	   =	${ChopBin_o} \
				${BinCluster_o} \
				${StaticsticUnique_o}\
				${Sam2ReadOnContig_o}\
				${ContigDlink_o}\
				${FillContigRoad_o}\
				${MergeContig_o}\
				${LinearCDG_o}

all : ${jobs}

ChopBin : clean ${ChopBin_o}
	${CXX} ${DEUBG_CXX} ${ChopBin_o} -o ChopBin

BinCluster : clean ${BinCluster_o}
	${CXX} ${DEUBG_CXX} ${BinCluster_o} -o BinCluster


StaticsticUnique : clean ${StaticsticUnique_o}
	${CXX} ${DEUBG_CXX} ${StaticsticUnique_o} -o StaticsticUnique


Sam2ReadOnContig: clean ${Sam2ReadOnContig_o}
	${CXX} ${DEUBG_CXX} ${Sam2ReadOnContig_o} -o Sam2ReadOnContig 

ContigDlink: clean ${ContigDlink_o}
	${CXX} ${DEUBG_CXX} ${ContigDlink_o} -o ContigDlink

FillContigRoad: clean ${FillContigRoad_o}
	${CXX} ${DEUBG_CXX} ${FillContigRoad_o} -o FillContigRoad 

MergeContig : clean ${MergeContig_o}
	${CXX} ${DEUBG_CXX} ${MergeContig_o} -o MergeContig

LinearCDG: clean ${LinearCDG_o}
	${CXX} ${DEUBG_CXX} ${LinearCDG_o} -o LinearCDG 

dirty	   =${jobs_o} ${jobs}  

clean:
	rm -rf ${dirty}