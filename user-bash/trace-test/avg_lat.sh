cat print.out|awk '{sum+=$2} END {print "Average = ", sum/NR}'
