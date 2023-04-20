#!/bin/bash
make client

if [ $? -eq 0 ]; then
    ./fget GET example1.txt
    ./fget GET
    ./fget GET example1.txt resp1.txt
    ./fget GET example1
    ./fget MD 
    ./fget MD put.txt
    ./fget MD test1
    ./fget MD test2
    ./fget MD test3
    ./fget MD test1/subtest1
    ./fget PUT test_put.txt test1/subtest1/put.txt
    ./fget PUT example1.txt test2/example.txt
    ./fget PUT test_put.txt test_remove.txt
    ./fget INFO test1
    ./fget INFO test1/subtest1/put.txt
    ./fget INFO test2/example.txt
    ./fget RM test_remove.txt
    ./fget RM test3
else 
    echo "Compilation failed."
fi 
