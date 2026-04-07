#!/bin/bash
echo "Testing builtins"

# Help and exit
printf "help\nexit\n" | ./build/processforge

# cd and pwd
printf "pwd\ncd docs\npwd\nexit\n" | ./build/processforge
