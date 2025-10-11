from hatchling.builders.hooks.plugin.interface import BuildHookInterface
from setuptools import Extension
from setuptools.command.build_ext import build_ext
from distutils.dist import Distribution
import os
import sys
import shutil

class CustomBuildHook(BuildHookInterface):
  def initialize(self, version, build_data):
    if self.target_name != "wheel":
      return

    # Define the C extension
    ext = Extension(
      'cijak._native',
      sources=['cijak/_native.c'],
      extra_compile_args=['-O3'] if sys.platform != 'win32' else ['/O2'],
    )

    # Create a minimal distribution for build_ext
    dist = Distribution({'ext_modules': [ext]})
    
    # Setup build_ext command
    build_ext_cmd = build_ext(dist)
    build_ext_cmd.finalize_options()
    
    # Build in a temporary directory
    build_ext_cmd.build_lib = 'build/lib'
    build_ext_cmd.build_temp = 'build/temp'
    
    try:
      # Build the extension
      build_ext_cmd.run()
      
      # Find the built extension
      ext_path = build_ext_cmd.get_ext_fullpath(ext.name)
      
      if os.path.exists(ext_path):
        # Copy to the correct location for packaging
        dest_dir = os.path.join('src', 'cijak')
        os.makedirs(dest_dir, exist_ok=True)
        
        ext_filename = os.path.basename(ext_path)
        dest_path = os.path.join(dest_dir, ext_filename)
        
        shutil.copy2(ext_path, dest_path)
        print(f"Built extension: {dest_path}")
      else:
        print(f"Warning: Extension not found at {ext_path}")
        
    except Exception as e:
      print(f"Warning: C extension build failed: {e}")
      print("Package will use pure Python fallback")