#!/bin/bash
# Script to run ECSE 541 final project parts (software-only and hw-sw partitioning) repeatedly.
# Ensure that this file is stored in the top-level directory "ecse541-final-project".

# NOTE: you may need to run "chmod u+x run_sizes.sh" to be able to execute this script!!

echo "Usage: ./run_sizes.sh addrL addrA size_min size_max loops"

size_min=$3
size_max=$4
loops=$5

# memory directory
search_dir="./memory_files/memory"


for ((i = $size_min ; i <= $size_max; i++))
do
	# get the right memory file
	for entry in "${search_dir}/*"
	do
		# construct filename
		file_extension=".txt"
		search_str="$i""x""$i"$file_extension # e.g. 7x7.txt
		echo "Searching for file ending in $search_str..."
		
		# find filename that has the correct size
		filepath=$(find $search_dir -name "*$search_str*")

		# now run software and hardware (you may need to change the filenames 
		# depending on your makefile
		echo "Found file: $filepath"
	        echo "Size = $i"
	        ./Software_Only/chol $filepath $1 $2 $i $5
	        ./Inner_Accel/chol_hw $filepath $1 $2 $i $5
	done
done

