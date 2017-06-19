#cut -d ' ' -f 2-4 origin/FIU2_total/$1 > fio-trace/$1
#cut -d ' ' -f 2-4 origin/Cambridge/$1 > fio-trace/$1
dev="/dev/rbd$2"
echo "fio version 2 iolog" > fio-trace/$1
echo "$dev add" >> fio-trace/$1
echo "$dev open" >> fio-trace/$1
awk -vOFS=" " '{if($4=="W") {print "'$dev' write", $2, $3} else {print "'$dev' read", $2, $3}}' origin/Cambridge/$1 >> fio-trace/$1
#awk -vOFS=" " '{if($4=="W") {print "'$dev' write", $2, $3} else {print "'$dev' read", $2, $3}}' origin/FIU2_total/$1 >> fio-trace/$1
echo "$dev close" >> fio-trace/$1
cat fio-trace/$1 |awk 'BEGIN {max = 0} {if ($3+0>max+0) max=$3 fi} END {print "Max=", max}' > fio-trace/$1.info
cat fio-trace/$1 |grep 'write' |wc -l >> fio-trace/$1.info
cat fio-trace/$1 |grep 'read' |wc -l >> fio-trace/$1.info
#wc -l fio-trace/$1
