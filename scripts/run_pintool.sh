#!/bin/bash

PWD=`pwd`

PIN_ARGS=" -mt 1 -inline -slow_asserts -follow_execv -separate_memory"
PINTOOL_ARGS="-skip_int3"
# -filter_rtn <name>
# -filter_no_shared_libs

#setarch x86_64 
$CONCURRIT_TPDIR/pin/pin $PIN_ARGS -t $CONCURRIT_HOME/lib/instrumenter.so $PINTOOL_ARGS -- $@

cd $PWD
