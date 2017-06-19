#gcc -c -I/usr/include/python2.6 -I/usr/lib/python2.6/config iotrace.c iotrace_wrapper.c
#gcc -shared -o Iotrace.so iotrace.o iotrace_wrapper.o  /usr/lib/python2.6/config/libpython2.6.dll.a

gcc -c -fPIC -I/usr/include/python2.7 -I/usr/lib64/python2.7/config iotrace.c iotrace_wrapper.c
gcc -shared -fPIC -o Iotrace.so iotrace.o iotrace_wrapper.o #/usr/lib64/python2.7/config/libpython2.7.so
