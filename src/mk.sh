#!/bin/bash

gcc -o screenpen main.c glad.c -lGL -lXrandr -lX11 -std=c11
