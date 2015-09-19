#!/bin/sh

gcc -Wuninitialized -Wall demo.c serialize.c -ggdb -o testcseri
gcc -Wuninitialized -Wall example1.c serialize.c -ggdb -o cseriex1
gcc -Wuninitialized -Wall example2.c serialize.c -ggdb -o cseriex2
gcc -Wuninitialized -Wall example3.c serialize.c -ggdb -o cseriex3
