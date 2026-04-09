#!/bin/bash
set -euo pipefail

temp_dir="tests/temp"
mkdir -p "$temp_dir"
rm -f "$temp_dir/test.txt"

echo "Testing redirection"

printf "echo hello > $temp_dir/test.txt\nexit\n" | ./build/processforge
echo "Output file content:"
cat "$temp_dir/test.txt"

printf "echo world >> $temp_dir/test.txt\nexit\n" | ./build/processforge
echo "Appended file content:"
cat "$temp_dir/test.txt"

printf "cat < $temp_dir/test.txt\nexit\n" | ./build/processforge

rm -f "$temp_dir/test.txt"
