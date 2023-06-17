#!/bin/python
import argparse

def sphere(x):
    total = 0.0
    for x_i in x:
        total += x_i * x_i
    return total


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
            prog='sphere',
            description='Test program for optimization')
    parser.add_argument('vals', type=float, nargs="+")
    args = parser.parse_args()
    print(sphere(args.vals))

