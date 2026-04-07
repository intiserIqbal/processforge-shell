#!/bin/bash
echo "Testing pipe support"

# Simple pipeline
printf "ls | grep .c\nexit\n" | ./build/processforge

# Multi-stage pipeline
printf "ls | grep .c | sort | wc -l\nexit\n" | ./build/processforge

# Pipeline with fixtures
printf "cat tests/fixtures/input.txt | tr a-z A-Z | sort\nexit\n" | ./build/processforge
