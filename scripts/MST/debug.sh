#!/bin/bash
source conf.ini
# prepare scripts

MACRO=" -D xxx=$PREFIX \
   -D BIN=$EXEC_DIR \
   -D BWA=$BWA_DIR/bwa \
   -D BIN_SIZE=$BIN_SIZE \
   -D BIN_THRESHOLD=$BIN_CLUSTER \
   -D HT_THRESHOLD=$HT_CLUSTER \
   -D MST_THRESHOLD=$MST_CLUSTER \
   -D MMCONTIG=$MIN_SCONTIG
   -D KVALUE=$SOAP_K \
   -D R1=$R1 \
   -D R2=$R2 \
   -D RANK=$RANK \
   -D THREADS=$THREADS \
   -D TAIL=$TAIL_DEL "

echo "Generate all scripts ..."
m4 $MACRO $DATA/step_0_prepare_info.m4 >step_0_prepare_info.sh
m4 $MACRO $DATA/step_1_calc_seeds.m4 >step_1_calc_seeds.sh
m4 $MACRO $DATA/step_2_bin_cluster.m4 >step_2_bin_cluster.sh
m4 $MACRO $DATA/step_3_mst.m4 >step_3_mst.sh
m4 $MACRO $DATA/step_4_gap_oo.m4 >step_4_gap_oo.sh
m4 $MACRO $DATA/step_5_trunk2scaff.m4 >step_5_trunk2scaff.sh
m4 $MACRO $DATA/clean_prepare.m4 >clean_prepare.sh

chmod u+x *.sh 

echo "prepare.sh finish"
