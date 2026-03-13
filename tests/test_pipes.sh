#!/bin/bash

echo "Testing pipe support"

printf "ls | grep .c\nexit\n" | ./build/processforge