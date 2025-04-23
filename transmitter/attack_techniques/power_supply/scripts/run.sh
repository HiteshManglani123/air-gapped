#!/bin/bash

gcc src/*.c -I ./include/ -lm -o build/transmitter

build/transmitter
