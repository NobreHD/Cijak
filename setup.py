from setuptools import setup, Extension
import sys

extra_compile_args = []
if sys.platform != 'win32':
    extra_compile_args = ['-O3', '-Wall']
else:
    extra_compile_args = ['/O2']

setup(
    ext_modules=[
        Extension(
            'cijak._native',
            sources=['cijak/_native.c'],
            extra_compile_args=extra_compile_args,
            optional=True,  # Build won't fail if C compilation fails
        )
    ],
)