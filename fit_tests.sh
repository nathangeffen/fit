#!/bin/sh

./src/fit

./src/fit -h

./src/fit -m random -f sphere -i 500

./src/fit -m grid -f sphere

./src/fit -m nms -f sphere

./src/fit -m gradient -f sphere --dx sphere_dx

./src/fit -m grid -n 10 -g 4 -p 4 -d 4 --lo -10.0 --hi 10.0 -f external -c "fit_sphere"

./src/fit -m gradient -n 10 -g 4 -p 4 -d 4 --lo -10.0 --hi 10.0 -f external -c "fit_sphere" --dx external -y "fit_sphere_dx"
