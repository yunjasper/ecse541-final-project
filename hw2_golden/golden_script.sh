#!/bin/bash
# Script to run ECSE 541 HW2 golden model with loops but without actually implementing the loops

echo "Usage: ./golden_script.sh mem_file.txt  addrC addrA addrB size loops"

size=$5
loops=$6

for ((i = 0 ; i < loops; i++))
do
        echo "Loop $i"
        offset_addr=$(($size*$size*$i))
        addrC_eff=$(($2+$offset_addr))
        addrA_eff=$(($3+$offset_addr))
        addrB_eff=$(($4+$offset_addr))
	echo "offset_addr = $offset_addr  addrC_eff = $addrC_eff  addrA_eff = $addrA_eff  addrB_eff = $addrB_eff"

        ./main "$1" "$addrC_eff" "$addrA_eff" "$addrB_eff" "$size"
done

