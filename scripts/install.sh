#!/bin/bash

echo "Building ProcessForge..."

make

sudo cp build/processforge /usr/local/bin/processforge

echo "Installed! Run with: processforge"