#!/usr/bin/env python
import numpy

import setuptools
from setuptools import setup, Extension

__version__ = "0.1"

cargs = ['-fpermissive']


with open('README.md', 'r', encoding='utf-8') as f:
    long_description = f.read()

module1 = Extension('stridx', sources=['py_interf.cpp'], include_dirs=['.'], extra_compile_args=cargs,
language="c++",
)
                        
ext_modules = [module1]
                      
setup(
    name='stridx',
    version='1.0',
    setup_requires=['wheel'],
    python_requires='>=3',
    provides=['stridx'],
    description='Fast fuzzy string similarity search and indexing (for filenames) ',
    long_description=long_description,
    long_description_content_type='text/markdown',
    ext_modules=[module1]
)

