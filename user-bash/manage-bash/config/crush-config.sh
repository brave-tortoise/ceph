# get & decompile crushmap
#ceph osd getcrushmap -o crushmap.bin
#crushtool -d crushmap.bin -o crushmap.txt

# edit crushmap.txt
# ...

# complie & set crushmap
crushtool -c crushmap.ref -o crushmap.bin
ceph osd setcrushmap -i crushmap.bin
