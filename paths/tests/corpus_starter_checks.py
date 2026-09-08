#!/usr/bin/env python3
"""Independent arithmetic for authored starters; no runtime answer-key lookup.

Expressions below are developer-owned test code in authoring files, never player
input. Polynomial and rational computations use exact arithmetic; transcendental
probes use explicit tolerances and do not claim a proof of a limiting theorem.
"""
import ast
import builtins
import cmath
import importlib.util
import itertools
import json
import math
from fractions import Fraction
from math import *
from pathlib import Path
pow = builtins.pow

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('publish', ROOT / 'tools/generate_corpus_starters.py')
publish = importlib.util.module_from_spec(spec)
spec.loader.exec_module(publish)


class Polynomial:
    def __init__(self, terms):
        self.terms = {m: Fraction(c) for m, c in terms.items() if c}

    def __add__(self, other):
        terms = self.terms.copy()
        for m, c in other.terms.items():
            terms[m] = terms.get(m, 0) + c
        return Polynomial(terms)

    def __neg__(self):
        return Polynomial({m: -c for m, c in self.terms.items()})

    def __sub__(self, other):
        return self + -other

    def __mul__(self, other):
        result = Polynomial({})
        for a, x in self.terms.items():
            for b, y in other.terms.items():
                result += Polynomial({tuple(sorted(a + b)): x*y})
        return result

    def __truediv__(self, other):
        assert list(other.terms) == [()], 'Polynomial division is only by a scalar'
        return self * Polynomial({(): 1/other.terms[()]})

    def derivative(self, variable):
        result = Polynomial({})
        for m, c in self.terms.items():
            power = m.count(variable)
            if power:
                factors = list(m); factors.remove(variable)
                result += Polynomial({tuple(factors): power*c})
        return result

    def value(self, values):
        return sum(c*prod(values[v] for v in m) for m, c in self.terms.items())


def polynomial(text):
    def read(node):
        if isinstance(node, ast.Constant): return Polynomial({(): Fraction(str(node.value))})
        if isinstance(node, ast.Name): return Polynomial({(node.id,): 1})
        if isinstance(node, ast.UnaryOp) and isinstance(node.op, ast.USub): return -read(node.operand)
        if isinstance(node, ast.BinOp):
            left = read(node.left)
            if isinstance(node.op, ast.Pow):
                assert isinstance(node.right, ast.Constant) and 0 <= node.right.value <= 12
                result = Polynomial({(): 1})
                for _ in range(node.right.value): result = result * left
                return result
            right = read(node.right)
            return {ast.Add: lambda: left+right, ast.Sub: lambda: left-right,
                    ast.Mult: lambda: left*right, ast.Div: lambda: left/right}[type(node.op)]()
        raise AssertionError(f'Unsupported polynomial: {text}')
    return read(ast.parse(text, mode='eval').body)


def poly(a, b): return polynomial(a).terms == polynomial(b).terms
def derivative(a, b, variable='x'): return polynomial(a).derivative(variable).terms == polynomial(b).terms
def derivative_at(a, x): return polynomial(a).derivative('x').value({'x': x})
def degree_lc(a):
    terms = polynomial(a).terms
    degree = max(len(m) for m in terms)
    return degree, terms[('x',)*degree]
def roots(a): return [x for x in range(-20, 21) if polynomial(a).value({'x': x}) == 0]
def irreducible(a):
    # Monic quadratic over Q: irreducible iff it has no rational root. For the
    # specific x^2-2, the rational-root candidates divide the constant term.
    assert poly(a, 'x**2-2')
    return all(polynomial(a).value({'x': x}) != 0 for x in [-2, -1, 1, 2])
def integral(a, lo, hi):
    answer = Fraction(0)
    for m, coefficient in polynomial(a).terms.items():
        assert all(v == 'x' for v in m)
        p = len(m)+1
        answer += coefficient * Fraction(hi**p-lo**p, p)
    return answer
def matvec(a, v): return [sum(x*y for x, y in zip(row, v)) for row in a]
def transpose(a): return [list(col) for col in zip(*a)]
def matmul(a, b): return [[sum(x*y for x, y in zip(row, col)) for col in zip(*b)] for row in a]
def det2(a): return a[0][0]*a[1][1]-a[0][1]*a[1][0]
def dot(a, b): return sum(x*y for x, y in zip(a, b))
def norm(a): return sqrt(dot(a, a))
def projection(v, u): return [Fraction(dot(v, u), dot(u, u))*x for x in u]
def gram(v): return [[dot(a, b) for b in v] for a in v]
def rank(matrix):
    a = [[Fraction(v) for v in row] for row in matrix]; pivot = 0
    for col in range(len(a[0])):
        row = next((r for r in range(pivot, len(a)) if a[r][col]), None)
        if row is None: continue
        a[pivot], a[row] = a[row], a[pivot]
        scale = a[pivot][col]; a[pivot] = [v/scale for v in a[pivot]]
        for r in range(len(a)):
            if r == pivot: continue
            scale = a[r][col]; a[r] = [x-scale*y for x, y in zip(a[r], a[pivot])]
        pivot += 1
        if pivot == len(a): break
    return pivot
def pseudoinverse_diagonal(a): return [1/Fraction(x) if x else 0 for x in a]
def variance(a):
    mean = Fraction(sum(a), len(a))
    return sum((x-mean)**2 for x in a)/len(a)
def covariance(a, b):
    ma, mb = Fraction(sum(a), len(a)), Fraction(sum(b), len(b))
    return sum((x-ma)*(y-mb) for x, y in zip(a, b))/len(a)
def prime(n): return n >= 2 and all(n%d for d in range(2, isqrt(n)+1))
def radical_integer(n): return prod(p for p in range(2, n+1) if prime(p) and n%p == 0)
def nilpotents(n): return [a for a in range(n) if any(pow(a, k, n) == 0 for k in range(1, n+1))]
def order_additive(a, n): return next(k for k in range(1, n+1) if a*k%n == 0)
def orbit(permutation, start):
    result = []; current = start
    while current not in result: result.append(current); current = permutation[current-1]
    return result
def rotation_orbit(v):
    values = set()
    while v not in values: values.add(v); v = (-v[1], v[0])
    return len(values)
def close(a, b, tolerance=1e-7): return abs(a-b) < tolerance
def close_complex(a, b): return abs(a-b) < 1e-7
def integrate_numeric(f, a, b):
    n = 4000; h = (b-a)/n
    return h/3*(f(a)+f(b)+4*sum(f(a+h*i) for i in range(1,n,2))+2*sum(f(a+h*i) for i in range(2,n,2)))
def numeric_derivative(f, x): return (f(x+1e-5)-f(x-1e-5))/2e-5
def trig_double_cos(): return all(close(cos(2*x), 2*cos(x)**2-1) for x in [0, .3, 1, 2, 3])
def mean_square_cos():
    assert close(integrate_numeric(lambda x:cos(x)**2, -pi, pi)/(2*pi), .5)
    return Fraction(1, 2)
def circular(a, b): return [sum(a[j]*b[(k-j)%len(a)] for j in range(len(a))) for k in range(len(a))]
def contour_reciprocal():
    return integrate_numeric(lambda t:(1/cmath.exp(1j*t))*(1j*cmath.exp(1j*t)), 0, 2*pi)
def topological(order, edges): return all(order.index(a)<order.index(b) for a, b in edges)
def fibonacci(n):
    a, b = 0, 1
    for _ in range(n): a, b = b, a+b
    return a
def max_drawdown(values): return max(1-Fraction(x, max(values[:i+1])) for i, x in enumerate(values))
def threshold_patterns(n): return {tuple(int(x>=a) for x in range(n)) for a in range(n+1)}


def main():
    bank = publish.build(); results = []
    for q in bank['questions']:
        check = q['check']
        if check == 'rule:functor_composition':
            # A concrete set-function composition with distinct domains/codomains.
            f = {0: 2, 1: 3}; g = {2: 4, 3: 5}
            assert {x: g[f[x]] for x in f} == {0: 4, 1: 5}
        elif check == 'rule:verdier_zero':
            # This is author-reviewed against Stacks 13.6; a numerical test
            # would not establish a statement about a triangulated quotient.
            pass
        else:
            assert eval(check, globals()), f"{q['id']}: {check}"
        results.append({'id':q['id'], 'check':check, 'passed':None if check.startswith('rule:') else True,
                        'kind':'given_rule_application' if check.startswith('rule:') else 'calculation_probe'})
    assert len({q['id'] for q in bank['questions']}) == 278
    print(json.dumps({'count':len(results), 'checks':results}, indent=2))


if __name__ == '__main__': main()
