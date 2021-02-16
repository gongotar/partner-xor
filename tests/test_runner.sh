#!/bin/bash

ranks=$1
pn=$2
xn=$3
config=$4

# get the checkpointing path
cppath=$(cat $config | grep "cp_path" | awk '{print $3}'| tr -d ';| ')

# configure the given partner and xor group sizes
ex -sc '%s/partner_group_size      = [0-9]*;/partner_group_size      = '${pn}';/g' -sc 'wq' $config
ex -sc '%s/xor_group_size          = [0-9]*;/xor_group_size          = '${xn}';/g' -sc 'wq' $config

rm -rf $cppath'/*'

mpirun -n $ranks combined_unit_test $config
