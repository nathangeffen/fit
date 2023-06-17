#!/bin/python

import math
import sys
import argparse
import random
import subprocess


def sphere(x):
    total = 0.0
    for x_i in x:
        total += x_i * x_i
    return total

def rastrigin(x):
    total = 10 * len(x)
    for i in x:
        term = (i + 10) * (i + 10) - 10 * math.cos(2 * math.pi * (i + 10))
        total += term
    return total

def external(x, command):
    args = [str(i) for i in x]
    result = float(subprocess.run([command, *args],
        capture_output=True, text=True).stdout)
    return result

def sysinout(x):
    out = " ".join(x)
    print(out)
    result = float(sys.stdin.readline())
    return


def make_domains(n, lo, hi):
    v = []
    if len(lo) == 1 and len(hi) == 1:
        for _ in range(n):
            v.append([lo[0], hi[0]])
    elif len(lo) == n and len(hi) == n:
        for i in range(n):
            v.append([lo[i], hi[i]])
    elif len(lo) == n and len(hi) == 1:
        for i in range(n):
            v.append([lo[i], hi[0]])
    elif len(lo) == 1 and len(hi) == n:
        for i in range(n):
            v.append([lo[0], hi[i]])
    else:
        raise ValueError("lo and hi must either have 1 value or the same "
                "number of values as the number of variables.")
    return v


class Counter:

    def __init__(self, max_vals):
        self.max_vals = max_vals
        self.counters = [0] * len(self.max_vals)

    def inc(self):
        for i in range(len(self.max_vals) - 1, -1, -1):
            self.counters[i] += 1
            if self.counters[i] >= self.max_vals[i]:
                self.counters[i] = 0
            else:
                break

    def rand(self):
        for i in range(len(self.max_vals)):
            self.counters[i] = random.randint(0, self.max_vals[i]-1)

    def product(self):
        total = 1
        for i in range(len(self.max_vals)):
            total *= self.max_vals[i]
        return total

    def reset(self):
        self.counters = [0] * len(self.max_vals)


class Optimization:
    FUNCTIONS = {
            'sphere': sphere,
            'rastrigin': rastrigin,
            'external': external
            }

    def __init__(self, method, func_str, domains, error=0.1,
            command=None, verbose=False, kwargs=None):
        self.method = method
        self.func_calls = 0
        self.function = self.FUNCTIONS[func_str]
        self.command = command
        self.domains = domains
        self.error = error
        self.kwargs = kwargs
        if verbose:
            print(f"method: {method}")
            print(f"function: {func_str}")
            print(f"command:", command)
            print(f"domains: {domains}")
            print(f"error: {error}")
            if kwargs:
                for k,v in kwargs.items():
                    print(f"{k}:", v)

    def exec_func(self, x):
        self.func_calls += 1
        if self.command:
            return self.function(x, self.command)
        else:
            return self.function(x)

    def optimize(self):
        METHODS = {
                'random': self.random,
                'grid_evolve': self.grid_evolve
                }
        method = METHODS[self.method]
        return method(self.kwargs)

    def random(self, parameters):
        lowest = sys.maxsize
        best = []
        for i in range(parameters['iterations']):
            v = []
            for d in self.domains:
                v.append(random.uniform(d[0], d[1]))
            result = self.exec_func(v)
            if result < lowest:
                lowest = result
                best = v.copy()
                if result < self.error:
                    break
        return {'Result': best,
                'Min': lowest,
                'Calls': self.func_calls
                }

    def grid_evolve(self, parameters):
        dom = [[d[0], d[1]] for d in self.domains]
        lowest = lowest_ever = sys.maxsize
        best = best_ever = []

        for g in range(parameters['generations']):
            if g > 0:
                dom = [[best[i] - step_size[i], best[i] + step_size[i]]
                        for i in range(len(best))]
            step_size = [(dom[i][1] - dom[i][0]) /
                    parameters['divisions'][i]
                    for i in range(len(dom))]

            for p in range(parameters['passes']):
                begin = [dom[i][0] + p/parameters['passes'] * step_size[i]
                        for i in range(len(dom))]

                for i in range(len(dom)):
                    v = best[0:i] + [begin[i]] + \
                            [random.uniform(dom[j][0], dom[j][1])
                                    for j in range(i+1, len(dom) ) ]

                    lowest = sys.maxsize
                    best = []

                    for j in range(parameters['divisions'][i]):
                        result = self.exec_func(v)

                        if result < lowest:
                            lowest = result
                            best = v.copy()
                            if lowest < lowest_ever:
                                lowest_ever = lowest
                                best_ever = best.copy()

                            if lowest < self.error:
                                break

                        v[i] += step_size[i]

                    if lowest < self.error:
                        break

                if lowest < self.error:
                    break

            if lowest < self.error:
                break


        return {'Result': best_ever,
                'Min': lowest_ever,
                'Calls': self.func_calls
                }



def process_args():

    def optimization_method(s):
        if s in ['grid', 'grid_evolve', 'grid_stochastic_evolve', 'random']:
            return s
        raise ValueError("Unknown method: " + s)

    parser = argparse.ArgumentParser(
            prog='grid',
            description='Optimize functions using grid method')
    parser.add_argument('-v', '--verbose', action='store_true', default=False,
            help='Verbose output')
    parser.add_argument('-n', '--variables', type=int, default=0,
            help='Number of variables')
    parser.add_argument('-f', '--function', type=str, default="external",
            help='Function to optimize')
    parser.add_argument('-l', '--lo', type=float, default=[-100.0], nargs="+",
            help='Lowest number in domain of variables')
    parser.add_argument('--hi', type=float, default=[100.0], nargs="+",
            help='Highest number in domain of variables')
    parser.add_argument('-d', '--divisions', type=int, default=[5], nargs="+",
            help='Number of divisions per variable')
    parser.add_argument('-g', '--generations', type=int, default=3,
            help='Number of generations for grid_evolve method')
    parser.add_argument('-p', '--passes', type=int, default=1,
            help='Number of iterations to use to optimize')
    parser.add_argument('-i', '--iter', type=int, default=1000,
            help='Number of iterations to use to optimize')
    parser.add_argument('-e', '--error', type=float,
            default="0.01",
            help='Stop if error is <= this value')
    parser.add_argument('-m', '--method', type=optimization_method,
            default="grid_evolve",
            help='Optimization method: grid (default) or random')
    parser.add_argument('-c', '--command', type=str, default="",
            help='External program to run')

    args = parser.parse_args()
    variables = args.variables
    lo = args.lo
    hi = args.hi
    if variables == 0:
        variables = max(len(lo), len(hi))
    domains = make_domains(variables, lo, hi)

    divisions = args.divisions
    if len(divisions) == 1:
        divisions = [divisions[0]] * variables

    o = Optimization(args.method, args.function, domains, args.error,
            args.command, args.verbose,
            {
                'divisions': divisions,
                'generations': args.generations,
                'passes': args.passes,
                'iterations': args.iter
                })

    print(o.optimize())

if __name__ == "__main__":
    process_args()
