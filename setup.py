import glob
import sys
from typing import List

from setuptools import setup, Extension

CONFIG_VERSION = open("cquickjs/VERSION").read().strip()
extra_link_args: List[str] = []

if sys.platform == "win32":
    # To build for Windows:
    # 1. Install MingW-W64-builds from https://mingw-w64.org/doku.php/download
    #    It is important to change the default to 64-bit when installing if a
    #    64-bit Python is installed in windows.
    # 2. Put the bin/ folder inside x86_64-8.1.0-posix-seh-rt_v6-rev0 in your
    #    system PATH when compiling.
    # 3. The code below will moneky-patch distutils to work.
    import distutils.cygwinccompiler
    distutils.cygwinccompiler.get_msvcr = lambda: [] 
    # Make sure that pthreads is linked statically, otherwise we run into problems
    # on computers where it is not installed.
    extra_link_args = ['-static']
    extra_library_args=['ws2_32']


def get_c_sources(include_headers=False):
    sources = [
        "module.c",
        "cquickjs/cutils.c",
        "cquickjs/libbf.c",
        "cquickjs/libregexp.c",
        "cquickjs/libunicode.c",
        "cquickjs/quickjs.c",
        "cquickjs/quickjs-debugger.c"
    ]
    if sys.platform == "win32":
        sources.append("cquickjs/quickjs-debugger-transport-win.c")
    else :
        sources.append("cquickjs/quickjs-debugger-transport-unix.c")

    if include_headers:
        sources += [
            "cquickjs/cutils.h",
            "cquickjs/libbf.h",
            "cquickjs/libregexp-opcode.h",
            "cquickjs/libregexp.h",
            "cquickjs/libunicode-table.h",
            "cquickjs/libunicode.h",
            "cquickjs/list.h",
            "cquickjs/quickjs-atom.h",
            "cquickjs/quickjs-opcode.h",
            "cquickjs/quickjs.h",
            "cquickjs/config.h",
            "cquickjs/quickjs-debugger.h",
            "cquickjs/VERSION",
        ]
    return sources


_quickjs = Extension(
    '_quickjs',
    define_macros=[('CONFIG_VERSION', f'"{CONFIG_VERSION}"'), ('CONFIG_BIGNUM', None)],
    # HACK.
    # See https://github.com/pypa/packaging-problems/issues/84.
    sources=get_c_sources(include_headers=("sdist" in sys.argv)),
    extra_compile_args=[],
	libraries=extra_library_args,
    extra_link_args=extra_link_args)

long_description = """
Python wrapper around https://bellard.org/quickjs/ .

Translates types like `str`, `float`, `bool`, `list`, `dict` and combinations
thereof to and from Javascript.

QuickJS is currently thread-hostile, so this wrapper makes sure that all calls
to the same JS runtime comes from the same thead.
"""

setup(author="Petter Strandmark",
      author_email="petter.strandmark@gmail.com",
      maintainer="Quentin Wenger",
      maintainer_email="matpi@protonmail.ch",
      name='quickjs',
      url='https://github.com/PetterS/quickjs',
      version='1.19.4',
      description='Wrapping the quickjs C library.',
      long_description=long_description,
      packages=["quickjs"],
      ext_modules=[_quickjs])
