# Cijak

[![PyPI Version](https://img.shields.io/pypi/v/cijak.svg)](https://pypi.org/project/cijak/)
[![Python Versions](https://img.shields.io/pypi/pyversions/cijak.svg)](https://pypi.org/project/cijak/)
[![](https://img.shields.io/badge/License-GPLv3%2B-blue.svg)](https://www.gnu.org/licenses/gpl-3.0)

A high density Unicode-based encoding library that packs binary data into CJK characters. Cijak encodes up to 14 bits per character, making it significantly more character-efficient than Base64 while maintaining competitive performance through optimized C extensions.

The name "Cijak" comes from the **C**JK Unicode block, where you can encode **14** bits of data (**C**1**J**4**K**).

## Why Cijak?

Cijak is designed for scenarios where:
- Character count matters more than byte size (SMS, tweets, chat apps)
- You need Unicode-safe encoding that doesn't look like obvious Base64

### Performance Comparison

Benchmarked on 3.7 KB of binary data:

|        Metric         | Base64  |  Cijak      | Cijak (Python Fallback) |
|-----------------------|---------|-------------|-------------------------|
| Encode                | 3.78 μs | 4.21 μs     | 1340 μs                 |
| Decode                | 8.19 μs | 5.24 μs     | 1338 μs                 |
| Character count       | 4,944   | 2,120       | ---                     |
| Character compression | -33%    | **43%**     | ---                     |
| UTF-8 size            | 4,944 B | 6,360 B     | ---                     |
| UTF-16 size           | 9,890 B | **4,242 B** | ---                     |

**TL;DR**: Cijak uses **57% fewer characters** than the original, while Base64 increases character count by 33%. Encoding speed matches Base64, decoding is faster.

**Note on Byte Size (UTF-8 vs UTF-16)**:

Since CJK characters typically use 3 bytes in UTF-8 but only 2 bytes in UTF-16 (or UCS-2), Cijak is highly byte-efficient only in environments that use UTF-16 (like many older Windows/Java internal systems) or where character limits are strictly enforced (like SMS). In standard UTF-8 environments, Base64 is often smaller in total byte size, but Cijak still uses 57% fewer characters.

## Installation

```bash
pip install cijak
```

Pre-compiled C extensions (wheels) are available for:
- **Linux** (x86_64, aarch64)
- **macOS** (Intel, Apple Silicon)  
- **Windows** (x86_64)
- **Python 3.8-3.14**

If a wheel isn't available, the package automatically falls back to a pure Python implementation (~300x slower, but functionally identical).

## Quick Start

```python
from cijak import Cijak

# Initialize encoder (uses CJK Unicode block by default)
encoder = Cijak()

# Encode binary data
data = b'Hello, World!'
encoded = encoder.encode(data)
print(encoded)  # ㇈怙擆羼稠蔦羐漀

# Decode back to bytes
decoded = encoder.decode(encoded)
print(decoded)  # b'Hello, World!'
```

## Advanced Configuration

### Custom Unicode Ranges

You can use different Unicode blocks for encoding:

```python
# Use a different range (e.g., Hangul)
encoder = Cijak(
    unicode_range_start=0xAC00,  # Hangul Syllables start
    unicode_range_end=0xD7A3,    # Hangul Syllables end
    marker_base=0x3200           # Different marker range
)

data = b"Custom encoding!"
encoded = encoder.encode(data)
```

**Important**: The Unicode range must not contain control characters. The library automatically calculates the optimal bit-packing based on your range size.

## Technical Deep Dive

### Encoding Scheme
- **Default**: CJK Unified Ideographs (U+4E00 to U+9FFF)
- **Bit Density**: 14 bits/character (calculated from range size)
- **Padding Marker**: A single character (base U+31C0) stores the number of padding bits required for decoding.
- **Efficiency**: ~1.75 bytes per character (vs Base64's 0.75)

### How It Works
1. Binary data is read as a continuous stream and packed into 14-bit chunks
2. Each chunk is mapped to a CJK codepoint
3. First character is a marker indicating padding
4. Remaining bits are left-padded in the last character

### Performance Notes
- **C extension**: Direct memory manipulation, zero Python overhead
- **Fallback**: BitReader/BitWriter abstraction in pure Python
- **Memory**: Single-pass encoding/decoding, minimal allocations
- **Thread-safe**: No global state, safe for concurrent use

## Building from Source

If you need to build the C extension manually:

```bash
git clone https://github.com/NobreHD/Cijak.git
cd Cijak
pip install -e .
```

Requirements:
- C compiler (GCC, Clang, MSVC)
- Python development headers

## Contributing

Contributions welcome! Areas of interest:
- SIMD optimizations for bulk encoding
- Additional Unicode range presets
- Streaming API for large files
- Alternative padding schemes

## License

GNU General Public License v3.0 or later (GPLv3+)