#!/bin/bash

# Test configuration
max_xor_group_size=8
max_partner_group_size=3
max_ranks=16
min_data_size=$1
max_data_size=$2
step_data_size=$3

# exit when any command fails
set -e

config=$4

# get the checkpointing path
cppath=$(cat $config | grep "cp_path" | awk '{print $3}'| tr -d ';| ')

pn=2
while [ $pn -le $max_partner_group_size ]; do
    xn=3
    while [ $xn -le $max_xor_group_size ]; do
        data=$min_data_size;
        while [ $data -le $max_data_size ]; do

            # check to see if we are allowed to run this test
            ranks=$(($xn*$pn))
            if [ $ranks -gt $max_ranks ]; then
                break
            fi

            # configure the given partner and xor group sizes
            ex -sc '%s/partner_group_size      = [0-9]*/partner_group_size      = '${pn}'/g' -sc 'wq' $config
            ex -sc '%s/xor_group_size          = [0-9]*/xor_group_size          = '${xn}'/g' -sc 'wq' $config

            # initialize environment
            rm -rf $cppath'/*'
            mkdir -p $cppath

            # perform the test
            echo "Performing $pn x $xn: mpirun -n $ranks combined_unit_test $data $config"
            mpirun -n $ranks combined_unit_test $data $config

            # in case of errors, return the error code
            ret=$?
            if [ $ret -ne 0 ]; then
                exit $ret
            fi

            data=$(($data+$step_data_size))
        done;
        xn=$(($xn+1))
    done;
    pn=$(($pn+1))
done;
