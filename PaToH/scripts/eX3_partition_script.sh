#!/bin/bash

initial_input=$1

remove_extension="${initial_input%%.*}"
remove_address="${remove_extension##*/}"

gunzip -c $1 > $PWD/$remove_address.mtx
unzipped_address="$PWD/$remove_address.mtx"

executable_name="a.out"

partition_quality="quality"

matrix_name_piece="${initial_input%%.*}"
matrix_name="${matrix_name_piece##*/}"

group_name_piece=$"${matrix_name_piece%/*}"
group_name_piece=$"${group_name_piece%/*}"

group_name=$"${group_name_piece##*/}"

folder_name="PaToH_${matrix_name}_${group_name}_${partition_quality}_partitioning_results"

mkdir "${folder_name}"
mv $executable_name $folder_name
cd $folder_name

./$executable_name $initial_input conpart 2 $partition_quality 1
./$executable_name $initial_input conpart 4 $partition_quality 1
./$executable_name $initial_input conpart 8 $partition_quality 1
./$executable_name $initial_input conpart 16 $partition_quality 1
./$executable_name $initial_input conpart 32 $partition_quality 1
./$executable_name $initial_input conpart 64 $partition_quality 1
./$executable_name $initial_input conpart 128 $partition_quality 1

./$executable_name $initial_input cutpart 2 $partition_quality 1
./$executable_name $initial_input cutpart 4 $partition_quality 1
./$executable_name $initial_input cutpart 8 $partition_quality 1
./$executable_name $initial_input cutpart 16 $partition_quality 1
./$executable_name $initial_input cutpart 32 $partition_quality 1
./$executable_name $initial_input cutpart 64 $partition_quality 1
./$executable_name $initial_input cutpart 128 $partition_quality 1
 
mv $executable_name ../
rm $unzipped_address

cd ../
mkdir -p Time_Info
mv ${folder_name}**/*timeinfo.txt Time_Info
