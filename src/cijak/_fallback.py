"""Pure Python fallback implementation of Cijak."""
import math

class BitReader:
  __slots__ = ("data", "bit_length", "position")

  def __init__(self, data: bytes):
    if not isinstance(data, (bytes, bytearray)):
      raise TypeError("BitReader expects bytes or bytearray")
    self.data = int.from_bytes(data, 'big')
    self.bit_length = len(data) * 8
    self.position = 0

  def read(self, bits: int) -> int:
    pos = self.position
    end = pos + bits
    if end > self.bit_length:
      raise ValueError("Not enough bits left to read")
    self.position = end
    return (self.data >> (self.bit_length - end)) & ((1 << bits) - 1)

  def remaining(self) -> int:
    return self.bit_length - self.position


class BitWriter:
  __slots__ = ("data", "bit_length")

  def __init__(self):
    self.data = 0
    self.bit_length = 0

  def write(self, value: int, bits: int):
    self.data = (self.data << bits) | value
    self.bit_length += bits

  def to_bytes(self) -> bytes:
    if self.bit_length == 0:
      return b""
    byte_length = (self.bit_length + 7) // 8
    return self.data.to_bytes(byte_length, 'big')


class Cijak:
  __slots__ = (
    "_unicode_range_start",
    "_unicode_range_end",
    "_bit_range",
    "_marker_base",
    "_mask",
  )

  def __init__(self, unicode_range_start=0x4E00, unicode_range_end=0x9FFF, marker_base=0x31C0):
    if unicode_range_start >= unicode_range_end:
      raise ValueError("Unicode range start must be less than end.")
    self._unicode_range_start = unicode_range_start
    self._unicode_range_end = unicode_range_end
    self._bit_range = math.floor(math.log2(unicode_range_end - unicode_range_start + 1))
    if not 1 <= self._bit_range <= 16:
      raise ValueError("Bit range needs to be between 1 and 16")
    self._marker_base = marker_base
    self._mask = (1 << self._bit_range) - 1

  @property
  def unicode_range_start(self):
    return self._unicode_range_start
  
  @property
  def bit_range(self):
    return self._bit_range
  
  @property
  def marker_base(self):
    return self._marker_base

  def encode(self, data: bytes) -> str:
    if not isinstance(data, (bytes, bytearray)):
      raise TypeError("Input data must be bytes or bytearray.")
    if not data:
      return ""

    bit_reader = BitReader(data)
    append = list.append 
    start = self._unicode_range_start
    bit_range = self._bit_range
    mask = self._mask

    chars = []
    total_bits = bit_reader.bit_length

    while bit_reader.remaining() >= bit_range:
      val = (bit_reader.data >> (total_bits - bit_reader.position - bit_range)) & mask
      bit_reader.position += bit_range
      append(chars, chr(start + val))

    rem = bit_reader.remaining()
    padding = (bit_range - rem) % bit_range
    if rem:
      val = ((bit_reader.data & ((1 << rem) - 1)) << padding)
      append(chars, chr(start + val))

    return chr(self._marker_base + padding) + "".join(chars)

  def decode(self, data: str) -> bytes:
    if not isinstance(data, str):
      raise TypeError("Input data must be a string.")
    if len(data) < 2:
      return b""

    marker = ord(data[0])
    padding = marker - self._marker_base
    
    if not (0 <= padding <= self._bit_range):
      raise ValueError("Invalid marker")
    
    bit_range = self._bit_range
    start = self._unicode_range_start

    bit_writer = BitWriter()
    write = bit_writer.write 

    for c in data[1:-1]:
      write(ord(c) - start, bit_range)

    last_val = ord(data[-1]) - start
    if padding:
      last_val >>= padding
    write(last_val, bit_range - padding)

    return bit_writer.to_bytes()