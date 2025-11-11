try:
  from cijak._native import Cijak
  __version__ = "1.0.1+native"
except ImportError:
  from cijak._fallback import Cijak
  __version__ = "1.0.1+fallback"
  import warnings
  warnings.warn("Using pure Python fallback (300x slower). Install build tools for C extension.")

__all__ = ['Cijak']