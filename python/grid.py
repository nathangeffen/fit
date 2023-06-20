#!/usr/bin/env python3

import argparse
import gettext
import math
import multiprocessing
import random
import subprocess
import sys
import threading

_ = gettext.gettext


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
        raise ValueError(_("lo and hi must either have 1 value or the same "
                         "number of values as the number of variables."))
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
                 command=None, verbose=False, max_threads=1, kwargs=None):
        self.method = method
        self.func_calls = 0
        self.function = self.FUNCTIONS[func_str]
        self.command = command
        self.domains = domains
        self.error = error
        self.kwargs = kwargs
        self.lock = threading.Lock()
        self.max_threads = max_threads
        if verbose:
            print(_("method: %(method)s" % {"method": method}))
            print(_("function: %(func_str)s" % {"func_str": func_str}))
            print(_("command: %(command)s" % {"command": command}))
            print(_("domains: %(domains)s" % {"domains": str(domains)}))
            print(_("error: %(error)f" % {"error": error}))
            if kwargs:
                for k, v in kwargs.items():
                    print(_("%(key)s: %(value)s" % {'key': k,
                                                    'value': str(v)}))
        if kwargs and "divisions" in kwargs:
            if len(kwargs["divisions"]) > 1 and \
                    len(kwargs["divisions"]) != len(domains):
                raise ValueError(_("Number of divisions must be 1 or equal to "
                                 "number of domains."))


    def exec_func(self, x):
        self.func_calls += 1
        if self.function == external:
            return self.function(x, self.command)
        else:
            return self.function(x)

    def optimize(self):
        METHODS = {
                'random': self.random,
                'grid': self.grid
                }
        method = METHODS[self.method]
        return method(self.kwargs)

    def random(self, parameters):
        lowest = sys.maxsize
        best = []
        for _ in range(parameters['iterations']):
            v = []
            for d in self.domains:
                v.append(random.uniform(d[0], d[1]))
            result = self.exec_func(v)
            if result < lowest:
                lowest = result
                best = v.copy()
                if result < self.error:
                    break
        return {'vector': best,
                'func': lowest,
                'calls': self.func_calls
                }


    def single_pass(self, dom, divisions, pass_no, pass_total,
                    step_size, result):
        begin = [dom[i][0] + pass_no/pass_total * step_size[i]
                 for i in range(len(dom))]
        best = []
        for i in range(len(dom)):
            v = best[0:i] + [begin[i]] + \
                    [random.uniform(dom[j][0], dom[j][1])
                     for j in range(i+1, len(dom))]
            lowest = sys.maxsize
            best = []
            for _ in range(divisions[i]):
                f = self.exec_func(v)
                if f < lowest:
                    lowest = f
                    best = v.copy()
                    self.lock.acquire()
                    if lowest < result['lowest_ever']:
                        result['lowest_ever'] = lowest
                        result['best_ever'] = best.copy()
                    self.lock.release()
                    if result['lowest_ever'] < self.error:
                        return
                v[i] += step_size[i]


    def grid(self, parameters):
        dom = [[d[0], d[1]] for d in self.domains]
        result = {
            'lowest_ever': sys.maxsize,
            'best_ever': []
        }
        step_size = []
        passes = parameters['passes']
        for g in range(parameters['generations']):
            if g > 0:
                dom = [[result['best_ever'][i] - step_size[i],
                        result['best_ever'][i] + step_size[i]]
                        for i in range(len(self.domains))]
            step_size = [(dom[i][1] - dom[i][0]) /
                         parameters['divisions'][i]
                         for i in range(len(dom))]
            p = 0
            while p < passes and \
                    result['lowest_ever'] > self.error:
                t = 0
                threads = []
                while t < self.max_threads and \
                        p < passes and \
                            result['lowest_ever'] > self.error:
                    threads.append(
                            threading.Thread(
                                target=self.single_pass,
                                    args=(dom, parameters['divisions'], p,
                                    passes, step_size, result)))
                    threads[-1].start()
                    t += 1
                    p += 1
                for thread in threads:
                    thread.join()
        return {'vector': result['best_ever'],
                'func': result['lowest_ever'],
                'calls': self.func_calls}


def process_args():

    def optimization_method(s):
        if s in ['grid', 'random']:
            return s
        raise ValueError(_("Unknown method: %(opt)s" % {'opt': s}))

    parser = argparse.ArgumentParser(
            prog='grid',
            description='Optimize functions using grid method')
    parser.add_argument('-v', '--verbose', action='store_true', default=False,
                        help=_('verbose output'))
    parser.add_argument('-n', '--variables', type=int, default=0,
                        help=_('number of variables'))
    parser.add_argument('-f', '--function', type=str, default="external",
                        help=_('function to optimize'))
    parser.add_argument('-l', '--lo', type=float, default=[-100.0], nargs="+",
                        help=_('lowest number in domain of variables'))
    parser.add_argument('--hi', type=float, default=[100.0], nargs="+",
                        help=_('highest number in domain of variables'))
    parser.add_argument('-d', '--divisions', type=int, default=[5], nargs="+",
                        help=_('number of divisions per variable'))
    parser.add_argument('-g', '--generations', type=int, default=3,
                        help=_('number of generations for grid_evolve method'))
    parser.add_argument('-p', '--passes', type=int, default=1,
                        help=_('number of passes per division in grid optimize'))
    parser.add_argument('-t', '--threads', type=int,
                        default=multiprocessing.cpu_count(),
                        help=_('maximum number of threads to use'))
    parser.add_argument('-i', '--iter', type=int, default=1000,
                        help=_('number of iterations to use to optimize'))
    parser.add_argument('-e', '--error', type=float, default="0.01",
                        help=_('stop if error is <= this value'))
    parser.add_argument('-m', '--method', type=optimization_method,
                        default="grid",
                        help=_('optimization method: '
                               'grid (default) or random'))
    parser.add_argument('-c', '--command', type=str, default="./sphere",
                        help=_('external program to run'))

    args = parser.parse_args()
    variables = args.variables
    lo = args.lo
    hi = args.hi
    if variables == 0:
        variables = max(len(lo), len(hi))
    try:
        domains = make_domains(variables, lo, hi)
        divisions = args.divisions
        if len(divisions) == 1:
            divisions = [divisions[0]] * variables
        o = Optimization(args.method, args.function, domains, args.error,
                         args.command, args.verbose, args.threads,
                         kwargs = {
                            'divisions': divisions,
                            'generations': args.generations,
                            'passes': args.passes,
                            'iterations': args.iter
                          })

        result = o.optimize()
        print(_("Best vector: %(vector)s" % {"vector": str(result['vector'])}))
        print(_("Minimum found: %(func)f" %
                {"func": result["func"]}))
        print(_("Function calls: %(calls)d" %
                {'calls': result['calls']}))
    except ValueError as err:
        print(_("Error: %(msg)s" % {'msg': err.args[0]}))
        raise


if __name__ == "__main__":
    process_args()
