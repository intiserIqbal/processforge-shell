#!/bin/bash
echo "Testing redirection"

# Output redirection
printf "echo hello > test.txt\nexit\n" | ./build/processforge

echo "Output file content:"
cat test.txt

# Append redirection
printf "echo world >> test.txt\nexit\n" | ./build/processforge

echo "Appended file content:"
cat test.txt

# Input redirection
printf "cat < test.txt\nexit\n" | ./build/processforge
