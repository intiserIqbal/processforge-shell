#!/bin/bash

echo "Testing redirection"

printf "echo hello > test.txt\nexit\n" | ./build/processforge