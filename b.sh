#!/bin/bash

a=10
b=20
c="this is a test"
d=$((a+b))
e=$((a-b))
f=$((a*b))
g=$((a/b))
h=$((a%b))

echo $c
echo "a+b = "${d}      #输出a+b的值
echo "a-b = "${e}      #输出a-b的值
echo "a*b = "${f}      #输出a*b的值
echo "a/b = "${g}      #输出a/b的值
echo "a%b = "${h}      #输出a%b的值