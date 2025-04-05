import math

class BitReader:
  def __init__(self, data: bytes):
    self.data = int.from_bytes(data, 'big')
    self.bit_length = len(data) * 8
    self.position = 0

  def can_read(self, bits: int) -> bool:
    return self.position + bits <= self.bit_length

  def read(self, bits: int) -> int:
    if not self.can_read(bits):
      raise ValueError("Not enough bits left to read")
    
    value = (self.data >> (self.bit_length - self.position - bits)) & ((1 << bits) - 1)
    self.position += bits
    return value
  
  def read_remaining_bits(self):
    remaining_bits = self.bit_length - self.position
    if remaining_bits == 0:
        return 0, 0
    
    return self.read(remaining_bits), remaining_bits

class BitWriter:
  def __init__(self):
    self.data = 0
    self.bit_length = 0

  def write(self, value: int, bits: int):
    self.data = (self.data << bits) | (value & ((1 << bits) - 1))
    self.bit_length += bits
  
  def to_bytes(self) -> bytes:
    byte_length = (self.bit_length + 7) // 8
    return self.data.to_bytes(byte_length, 'big')

class Cijak:
  def __init__(self, unicode_range_start=0x4E00, unicode_range_end=0x9FFF, marker_base=0x31C0):
    self.unicode_range_start = unicode_range_start
    self.bit_range = math.floor(math.log2(unicode_range_end - unicode_range_start + 1))
    self.marker_base = marker_base

  def encode(self, data: bytes) -> str:
    if not isinstance(data, bytes):
      raise TypeError("Input data must be bytes")

    bit_reader = BitReader(data)
    result = []

    while bit_reader.can_read(self.bit_range):
      result.append(chr(self.unicode_range_start + bit_reader.read(self.bit_range)))

    remaining_bits, size = bit_reader.read_remaining_bits()
    padding_bits = (self.bit_range - size) % self.bit_range
    if size > 0:
      remaining_bits <<= padding_bits
      result.append(chr(self.unicode_range_start + remaining_bits))

    return chr(self.marker_base + padding_bits) + "".join(result)

  def decode(self, data: str) -> bytes:
    if not isinstance(data, str):
      raise TypeError("Input data must be str")
    if not data:
      raise ValueError("Input data must not be empty")

    bit_writer = BitWriter()
    marker = ord(data[0])

    if not (self.marker_base <= marker <= self.marker_base + self.bit_range):
      raise ValueError("Invalid marker")

    padding_bits = marker - self.marker_base

    for char in data[1:-1]:
      bit_writer.write(ord(char) - self.unicode_range_start, self.bit_range)

    remaining_bits = ord(data[-1]) - self.unicode_range_start
    if padding_bits > 0:
      remaining_bits >>= padding_bits
    bit_writer.write(remaining_bits, self.bit_range - padding_bits)

    return bit_writer.to_bytes()
