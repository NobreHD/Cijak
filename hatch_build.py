from hatchling.builders.hooks.plugin.interface import BuildHookInterface
from setuptools import Extension
from setuptools.command.build_ext import build_ext
import os

class CustomBuildHook(BuildHookInterface):
  def initialize(self, version, build_data):
    if self.target_name not in ["wheel", "sdist"]:
      return

    # Define the C extension
    ext = Extension(
      'cijak._native',
      sources=['src/cijak/_native.c'],
      extra_compile_args=['-O3'],
    )

    # Build the extension if building wheel
    if self.target_name == "wheel":
      # Create build_ext instance
      build_ext_cmd = build_ext(self.build_config.distribution)
      build_ext_cmd.extensions = [ext]
      build_ext_cmd.finalize_options()
      
      # Build
      try:
        build_ext_cmd.run()
        
        # Find the built extension
        ext_path = build_ext_cmd.get_ext_fullpath(ext.name)
        if os.path.exists(ext_path):
          # Add to build artifacts
          build_data['force_include'][ext_path] = os.path.basename(ext_path)
      except Exception as e:
        print(f"Warning: C extension build failed: {e}")
        print("Falling back to pure Python implementation")
    
    # For sdist, include the C source
    elif self.target_name == "sdist":
      build_data['force_include']['src/cijak/_native.c'] = 'cijak/_native.c'