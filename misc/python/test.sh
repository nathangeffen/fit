#!/bin/sh

python test.py

./grid.py

./grid.py -l -100.0 -50.0 -75.0 -21.2 --hi 125 30.0 40.0 550.0 -d 40 -g 4 -p 4 -e 0.01 -i 20000 -m grid -f external -c ./sphere

./grid.py -l -100.0 -50.0 -75.0 -21.2 --hi 125 30.0 40.0 550.0 -d 40 -g 4 -p 4 -e 0.01 -i 20000 -m grid -f rastrigin


./grid.py -l -100.0 -50.0 -75.0 -21.2 --hi 125 30.0 40.0 550.0 -d 40 -g 4 -p 4 -e 0.01 -i 20000 -m grid -f sphere

./grid.py -m random -n 100 -l -103.0 --hi 125.0 -i 10000 -e 0.01 -f sphere
./grid.py -m random -n 100 -l -103.0 --hi 125.0 -i 10000 -e 0.01 -f rastrigin

./grid.py -n 100 -l -103.0 --hi 125.0 -d 40 -g 4 -p 4 -e 0.01 -f sphere
./grid.py -n 100 -l -103.0 --hi 125.0 -d 40 -g 4 -p 4 -e 0.01 -f rastrigin

