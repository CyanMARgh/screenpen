#!/bin/bash

gcc -o screenpen gl33.c glad.c -lGL -lXrandr -lX11 -std=c11
