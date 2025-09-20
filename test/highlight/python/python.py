#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Python syntax-highlighting stress test.
This file intentionally contains a very wide variety of Python language features,
idioms, punctuation, and corner cases so that syntax highlighters can be tested.
Do not use this file as production code — it's deliberately noisy.
"""

from __future__ import annotations, generator_stop

# Standard library imports (multiple styles)
import sys, os as _os, math
from collections import defaultdict, namedtuple as NT, deque
from dataclasses import dataclass, field, InitVar
from typing import (
    Any, Callable, Dict, Iterable, Iterator, List, Optional, Tuple, Union, TypeVar,
    Generic, Protocol, runtime_checkable, Final, NewType, Literal, Annotated
)
import asyncio
import re
import logging

# Shebang, encoding comment, and weird unicode identifier
λ = 3.14  # non-ascii variable name
EMOJI_VAR_💡 = 'emoji'

# Type constructs
T = TypeVar('T')
UserId = NewType('UserId', int)
MAX = Final[float](1.0)

LiteralChoice = Literal['a', 'b', 1]
AnnotatedInt = Annotated[int, "must be positive"]

# Comments with keywords: TODO, FIXME
# TODO: this is a test
# FIXME: nothing to fix

# Module-level constant with complex literal: underscores, imaginary
COMPLEX_CONST = 1_000_000 + 3.5j

# Multi-line and single-line docstrings
class _Hidden:
    """A hidden helper with a triple-quoted docstring and
    indentation that spans multiple lines.
    """
    pass

# DataClass with defaults, InitVar, slots, kw-only args, repr customization
@dataclass(slots=True)
class Person:
    id: UserId
    name: str = 'Anonymous'
    tags: List[str] = field(default_factory=list)
    _secret: InitVar[Optional[str]] = None

    def __post_init__(self, _secret: Optional[str]):
        if _secret:
            self.tags.append(f"secret:{_secret}")

    def __repr__(self) -> str:  # override repr
        return f"Person(id={self.id!r}, name={self.name!r})"


# Enum-like class and usage of bitwise ops, shifts
class Flags(int):
    READ = 0b0001
    WRITE = 0b0010
    EXEC = 0b0100


# Descriptor example
class ReprDescriptor:
    def __get__(self, obj, objtype=None):
        return f"<repr of {obj!r}>"


class Container:
    descr = ReprDescriptor()

    def __init__(self, *args, **kwargs):
        self._data = list(args)

    def __len__(self):
        return len(self._data)

    def __iter__(self):
        return iter(self._data)

    def __contains__(self, item):
        return item in self._data


# Metaclass, __new__, __init_subclass__
class VerboseMeta(type):
    def __new__(mcls, name, bases, ns):
        ns.setdefault('__created_with_meta__', True)
        return super().__new__(mcls, name, bases, ns)


class Base(metaclass=VerboseMeta):
    def __init_subclass__(cls, /, **kwargs):
        super().__init_subclass__(**kwargs)
        cls._subclass_marker = True


# Generator, generator delegation, send(), throw(), close()
def gen_example(n: int) -> Iterator[int]:
    yield from (i for i in range(n) if i % 2 == 0)


# Async features
async def coro_sleep_and_return(x: int) -> int:
    await asyncio.sleep(0)  # yield control
    return x * 2


async def async_iter_example(n: int):
    for i in range(n):
        yield i


# Context managers (sync + async)
from contextlib import contextmanager, asynccontextmanager

@contextmanager
def demo_ctx(x):
    try:
        yield f"enter:{x}"
    finally:
        pass

@asynccontextmanager
async def demo_async_ctx(x):
    try:
        yield f"aenter:{x}"
    finally:
        pass


# Regular expressions: raw strings, verbose, backrefs
RE_NUM = re.compile(r"(?P<int>\d+)(?:\.(?P<frac>\d+))?")
RE_VERBOSE = re.compile(r"(?x)  # verbose regex\n    (\d{1,3})  # 1-3 digits\n    (?:,\s*\d{3})*  # optional comma groups\")


# Complex f-strings, raw f-strings (Python f-strings don't accept r before f in older Pythons,
# but many highlighters should still accept f r combinations as tokens in either order)
name = 'Alice'
msg = f"Hello, {name!s} — value is {COMPLEX_CONST:.2f}"
raw_bytes = r"\n\t\\"

# Bytes and bytearray
b = b"\x00\xff"
br = bytearray(b)

# Lambdas, nested lambdas, default mutable argument (bad but intentionally present)
def make_adder(x, /, *, scale=1):
    return lambda y, z=0: (x + y + z) * scale


# Function with positional-only, keyword-only, varargs, kwargs, annotations, default factories
def fancy(a, b=2, /, c=3, *args: int, d: int = 4, e: int = 5, **kwargs) -> Tuple[int, ...]:
    """Fancy function demonstrating many parameter styles."""
    return (a + b + c + d + e + sum(args), kwargs.get('extra', 0))


# Walrus operator and complex assignment
def walrus_demo(seq: Iterable[int]):
    if (n := sum(seq)) > 10:
        return n
    return 0


# Pattern matching (structural) - Python 3.10+
def pattern_demo(value: Any) -> str:
    match value:
        case 0:
            return 'zero'
        case [x, y]:
            return f'pair:{x},{y}'
        case {'id': int(id_val)}:
            return f"has-id:{id_val}"
        case _:
            return 'other'


# Typed dict and Protocols (typing)
class SupportsClose(Protocol):
    def close(self) -> None: ...


# Complex comprehensions, unpacking, generator expressions
comp = [x**2 for x in range(10) if x % 2 == 0]
set_comp = {x for x in range(5)}
map_comp = {**{'a': 1}, **{'b': 2}}

nested_comp = [(i, j) for i, row in enumerate([[1, 2], [3, 4]]) for j in row]

# Extended slices
s = list(range(10))[1:8:2]

# Ellipsis and None used as literal tokens
ellipsis_example = (..., None)

# Multiple statements on one line, semicolons
x = 1; y = 2; z = x + y

# Chained comparisons and ternary
is_between = 1 < x < 5
tern = 'yes' if x > 0 else 'no'

# Numeric literals: binary, octal, hex, floats, complex
n_bin = 0b1010
n_oct = 0o755
n_hex = 0xDEAD_BEEF
n_float = 1.23e-4
n_complex = 3+4j

# Complex slicing with negative step
slice_example = tuple(range(-5, 5))[::-1]

# Exception handling variations
try:
    1 / 0
except ZeroDivisionError as e:
    logging.exception('division failed: %s', e)
except Exception:
    raise
finally:
    pass

# Raising with from
class MyError(Exception):
    pass

try:
    raise MyError('top')
except MyError as e:
    raise RuntimeError('wrapped') from e

# Use of assert and inspect
assert isinstance(name, str), 'name must be str'

# Dynamic code: eval/exec (strings included), compile
_dynamic_code = """
def dyn(x):
    return x * x
"""
exec(_dynamic_code, globals())

# Use of subprocess-like pattern (but not executing external code here)
CMD = ['echo', 'hello']

# File handling with encoding and weird newline
with open(__file__, 'r', encoding='utf-8', newline='') as _f:  # type: ignore
    _first_line = _f.readline()

# Complex data structures: nested dicts, tuples, frozenset
big = {
    'numbers': [1, 2, 3],
    'mapping': {'a': (1, 2, 3), 'b': {1, 2}},
    'frozen': frozenset({1, 2, 3})
}

# Unicode escapes inside strings and raw f-strings
unicode_str = "Unicode: \u2603"
raw_f = fr"path:\\home\\{name}"

# Decorators: stacking, classmethod, staticmethod, property
def deco(fn: Callable[[T], T]) -> Callable[[T], T]:
    def wrapper(*a, **k):
        return fn(*a, **k)
    return wrapper

@deco
@deco
def decorated(x: int) -> int:
    return x + 1

class WithProps:
    @property
    def read_only(self) -> str:
        return 'ro'

    @staticmethod
    def static_meth(x):
        return x * 2

    @classmethod
    def cls_meth(cls):
        return cls

# Operator overloading and rich comparisons
class Vector:
    def __init__(self, *coords: float):
        self.coords = coords

    def __add__(self, other):
        return Vector(*(a + b for a, b in zip(self.coords, other.coords)))

    def __repr__(self):
        return f"Vector{self.coords}"

    def __len__(self):
        return len(self.coords)


# Coroutine protocol: __await__
class AwaitableThing:
    def __await__(self):
        if False:
            yield
        return 42


# Async for/with and a complex async function with annotations
async def complex_async():
    async with demo_async_ctx('x') as ctx:
        async for i in async_iter_example(3):
            await asyncio.sleep(0)


# Main runner: argparse, logging, conditional imports
def main(argv: Optional[List[str]] = None) -> int:
    import argparse
    parser = argparse.ArgumentParser(description='Syntax highlight stress test')
    parser.add_argument('--count', '-c', type=int, default=3)
    parser.add_argument('--quiet', action='store_true')
    ns = parser.parse_args(argv)

    if not ns.quiet:
        logging.basicConfig(level=logging.DEBUG)
        logging.info('Running stress test')

    # exercises
    p = Person(UserId(1), 'Bob', _secret='s')
    c = Container(1, 2, 3)
    v = Vector(1, 2, 3)
    _ = decorated(10)
    _ = make_adder(5)(10)
    _ = walrus_demo([1, 2, 3, 4, 5])
    try:
        # pattern_demo tests
        _ = pattern_demo({'id': 123})
    except Exception:
        pass

    # demonstrate async run
    try:
        asyncio.run(coro_sleep_and_return(2))
    except RuntimeError:
        # nested event loop in REPL possible
        pass

    return 0


if __name__ == '__main__':
    raise SystemExit(main())
