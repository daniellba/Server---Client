#!/bin/bash

echo Test1-URL > res1.txt
valgrind -v --log-file=val1.txt ./client.o http://www.ptsv2.com/t/ex2 &>> res1.txt
echo Test2-Post > res2.txt
valgrind -v --log-file=val2.txt ./client.o -p inPost http://www.ptsv2.com/t/ex2 &>> res2.txt
echo Test3-Empty Post > usage.txt
valgrind -v --log-file=val3.txt ./client.o -p http://www.ptsv2.com/t/ex2 &>> usage.txt
echo Test4-Post 2 Words >> usage.txt
valgrind -v --log-file=val4.txt ./client.o -p on post http://www.ptsv2.com/t/ex2 &>> usage.txt
echo Test5-Post after URL > res5.txt
valgrind -v --log-file=val5.txt ./client.o http://www.ptsv2.com/t/ex2 -p post &>> res5.txt
echo Test6-URL without / > res6.txt
valgrind -v --log-file=val6.txt ./client.o http://www.google.com &>> res6.txt
echo Test7- Variables > res7.txt
valgrind -v --log-file=val7.txt ./client.o -r 2 addr=tel-aviv age=8 http://www.ptsv2.com/t/ex2 &>> res7.txt
echo Test8- Variables without number >> usage.txt
valgrind -v --log-file=val8.txt ./client.o -r age=8 http://www.ptsv2.com/t/ex2 &>> usage.txt
echo Test9- to much variables >> usage.txt
valgrind -v --log-file=val9.txt ./client.o -r 1 age=8 age2=9 http://www.ptsv2.com/t/ex2 &>> usage.txt
echo Test10- not enough variables >> usage.txt
valgrind -v --log-file=val10.txt ./client.o -r 3 age=8 age2=9 http://www.ptsv2.com/t/ex2 &>> usage.txt
echo Test11- post and variables > res11.txt
valgrind -v --log-file=val11.txt ./client.o -p inPost -r 2 addr=tel-aviv age=8 http://www.ptsv2.com/t/ex2 &>> res11.txt
echo Test12- empty post and variables >> usage.txt
valgrind -v --log-file=val12.txt ./client.o -p -r 2 addr=tel-aviv age=8 http://www.ptsv2.com/t/ex2 &>> usage.txt
echo Test13- empty variables and post >> usage.txt
valgrind -v --log-file=val13.txt ./client.o -r -p inPost http://www.ptsv2.com/t/ex2 &>> usage.txt
echo Test14-variables after URL > res14.txt
valgrind -v --log-file=val14.txt ./client.o http://www.google.com -r 1 age=8 &>> res14.txt
echo Test15- bad variables >> usage.txt
valgrind -v --log-file=val15.txt ./client.o -r 2 addr=tel-aviv age8 http://www.ptsv2.com/t/ex2 &>> usage.txt
echo Test16- no URL >> usage.txt
valgrind -v --log-file=val16.txt ./client.o -p inPost &>> usage.txt
echo Test17-URL with port > res17.txt
valgrind -v --log-file=val17.txt ./client.o http://www.jce.ac.il:8080/index.html &>> res17.txt
echo Test18- not enough arguments >> usage.txt
valgrind -v --log-file=val18.txt ./client.o &>> usage.txt
