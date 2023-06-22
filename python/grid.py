#!/usr/bin/env python3

"""
Reference implementation of Grid Evolve minimization algorithm.
See README.md for explanation of algorithm.

This program is free software: you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by the
Free Software Foundation, either version 3 of the License, or (at your
option) any later version.

This program is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
Public License for more details.

You should have received a copy of the GNU General Public License along
with this program. If not, see <https://www.gnu.org/licenses/>.  """

import argparse
import gettext
import math
import multiprocessing
import random
import subprocess
import sys

_ = gettext.gettext


# Test optimization functions

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

    """Optimization function that will typically be called when this program is
    invoked as a CLI.  """

    args = [str(i) for i in x]
    result = float(subprocess.run([command, *args],
        capture_output=True, text=True).stdout)
    return result


def make_domains(n, lo, hi):

    """Returns a list of 2-tuples, each being the lower and upper bound of the
    domain of each variable.  """

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


class Optimization:

    """Class to manage the optimization algorithms, and in which they are
    implemented.  """

    FUNCTIONS = {
            'sphere': sphere,
            'rastrigin': rastrigin,
            'external': external
            }

    def __init__(self, method, func_str, domains, error=0.1,
            command=None, verbose=False, max_jobs=1, kwargs=None):
        self.method = method
        self.func_calls = multiprocessing.Value('i')
        self.func_calls.value = 0
        self.function = self.FUNCTIONS[func_str]
        self.command = command
        self.domains = domains
        self.error = error
        self.kwargs = kwargs
        self.max_jobs = max_jobs
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

        """Calls the appropriate optimization function with the vector x, and
        increments the call count tracker. If an external program executes the
        function to be minimizaed then "external" is called with the
        command."""

        with self.func_calls.get_lock():
            self.func_calls.value += 1
        if self.function == external:
            return self.function(x, self.command)
        else:
            return self.function(x)

    def optimize(self):

        """ Call the appropriate optimization/minimization algorithm,
        currently, either random guession or the Grid Evolve algorithm.  """

        METHODS = {
                'random': self.random,
                'grid': self.grid
                }
        method = METHODS[self.method]
        return method(self.kwargs)

    def random(self, parameters):
        """Randomly attempts to find minimum of a function."""
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
                'calls': self.func_calls.value
                }

    def single_pass(self, dom, divisions, pass_no, pass_total, job_no,
            step_size, return_dict):

        """Iterates over each variable in the vector by a specified division
        choosing the set of values that minimizes the function.
        """

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
                    if lowest < self.error:
                        return_dict[job_no] = (lowest, best)
                        return
                v[i] += step_size[i]
        return_dict[job_no] = (lowest, best)

    def grid(self, parameters):

        """Grid Evolve minimization method that tries to find a global minimum
        for a function that takes a vector of floats and outputs a float.  """

        dom = [[d[0], d[1]] for d in self.domains]
        divisions = parameters['divisions']
        lowest_ever = sys.maxsize
        best_ever = []
        step_size = []
        passes = parameters['passes']
        for g in range(parameters['generations']):
            if g > 0:
                dom = [[best_ever[i] - step_size[i],
                        best_ever[i] + step_size[i]]
                       for i in range(len(self.domains))]
            step_size = [(dom[i][1] - dom[i][0]) / divisions[i]
                         for i in range(len(dom))]
            p = 0
            while p < passes and lowest_ever > self.error:
                j = 0
                jobs = []
                manager = multiprocessing.Manager()
                return_dict = manager.dict()
                while j < self.max_jobs and p < passes and \
                        lowest_ever > self.error:
                    jobs.append(multiprocessing.Process(
                                target=self.single_pass,
                                args=(dom, divisions, p, passes, j,
                                      step_size, return_dict)))
                    jobs[-1].start()
                    j += 1
                    p += 1
                for job in jobs:
                    job.join()
                for _, (l, b) in return_dict.items():
                    if l < lowest_ever:
                        lowest_ever = l
                        best_ever = b.copy()

        return {'vector': best_ever,
                'func': lowest_ever,
                'calls': self.func_calls.value}


def process_args():

    """Processes the command line arguments.
    """

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
                        help=_('number of passes per division in grid '
                                'optimize'))
    parser.add_argument('-j', '--jobs', type=int,
                        default=multiprocessing.cpu_count(),
                        help=_('maximum number of parallel jobs to run'))
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
                args.command, args.verbose, args.jobs,
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
