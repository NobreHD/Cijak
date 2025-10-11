from setuptools import setup, Extension
from setuptools.command.build_ext import build_ext
import sys

extra_compile_args = []
if sys.platform != 'win32':
    extra_compile_args = ['-O3', '-Wall']
else:
    extra_compile_args = ['/O2']


class BuildExtOptional(build_ext):    
    def build_extensions(self):
        try:
            super().build_extensions()
        except Exception as e:
            print(f"Warning: Failed to build C extension: {e}")
            print("Falling back to pure Python implementation")


setup(
    ext_modules=[
        Extension(
            'cijak._native',
            sources=['src/cijak/_native.c'],
            extra_compile_args=extra_compile_args,
            optional=True, 
        )
    ],
    cmdclass={'build_ext': BuildExtOptional},
)