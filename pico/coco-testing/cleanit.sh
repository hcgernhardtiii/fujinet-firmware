#!/bin/bash

find . -depth -type d -not -name '.vscode' -exec rm -r {} \;
rm *.elf
rm *.bin
rm *.map
rm *.dis
rm *.uf2
rm *.json
rm CMakeCache.txt
rm Makefile
rm cmake_install.cmake
rm pico_flash_region.ld
ls *.pio | while read i; do rm "$i.h"; done
