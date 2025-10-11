from cijak import Cijak
import unittest
import os

class TestCijakEdgeCases(unittest.TestCase): 
  def setUp(self):
    self.codec = Cijak()
  
  def test_empty_bytes(self):
    """Test encoding and decoding empty byte string"""
    data = b''
    encoded = self.codec.encode(data)
    decoded = self.codec.decode(encoded)
    self.assertEqual(decoded, data, "Empty bytes should round-trip correctly")
  
  def test_single_zero_byte(self):
    """Test single null byte"""
    data = b'\x00'
    encoded = self.codec.encode(data)
    decoded = self.codec.decode(encoded)
    self.assertEqual(decoded, data, "Single null byte should round-trip")
  
  def test_single_ff_byte(self):
    """Test single 0xFF byte"""
    data = b'\xff'
    encoded = self.codec.encode(data)
    decoded = self.codec.decode(encoded)
    self.assertEqual(decoded, data, "Single 0xFF byte should round-trip")
  
  def test_all_zeros(self):
    """Test various lengths of all-zero bytes"""
    for length in [1, 2, 3, 7, 8, 16, 100]:
      with self.subTest(length=length):
        data = b'\x00' * length
        encoded = self.codec.encode(data)
        decoded = self.codec.decode(encoded)
        self.assertEqual(decoded, data, f"All zeros (length {length}) should round-trip")
  
  def test_all_ones(self):
    """Test various lengths of all-0xFF bytes"""
    for length in [1, 2, 3, 7, 8, 16, 100]:
      with self.subTest(length=length):
        data = b'\xff' * length
        encoded = self.codec.encode(data)
        decoded = self.codec.decode(encoded)
        self.assertEqual(decoded, data, f"All 0xFF (length {length}) should round-trip")
  
  def test_alternating_pattern(self):
    """Test alternating 0xAA and 0x55 pattern"""
    data = b'\xaa\x55' * 50
    encoded = self.codec.encode(data)
    decoded = self.codec.decode(encoded)
    self.assertEqual(decoded, data, "Alternating pattern should round-trip")
  
  def test_sequential_bytes(self):
    """Test sequential byte values 0-255"""
    data = bytes(range(256))
    encoded = self.codec.encode(data)
    decoded = self.codec.decode(encoded)
    self.assertEqual(decoded, data, "Sequential 0-255 should round-trip")
  
  def test_random_data(self):
    """Test with cryptographically random data (simulates AES output)"""
    for length in [1, 7, 16, 32, 64, 127, 128, 1000]:
      with self.subTest(length=length):
        data = os.urandom(length)
        encoded = self.codec.encode(data)
        decoded = self.codec.decode(encoded)
        self.assertEqual(decoded, data, f"Random data (length {length}) should round-trip")
  
  def test_boundary_lengths(self):
    """Test lengths around bit_range boundaries"""
    bit_range = self.codec.bit_range
    byte_boundary = bit_range // 8
    
    for offset in [-2, -1, 0, 1, 2]:
      length = max(1, byte_boundary + offset)
      with self.subTest(length=length):
        data = os.urandom(length)
        encoded = self.codec.encode(data)
        decoded = self.codec.decode(encoded)
        self.assertEqual(decoded, data, f"Boundary length {length} should round-trip")
  
  def test_single_bit_differences(self):
    """Test that single bit flips are preserved"""
    base = b'\x00' * 10
    for byte_pos in range(10):
      for bit_pos in range(8):
        data = bytearray(base)
        data[byte_pos] |= (1 << bit_pos)
        data = bytes(data)
        
        with self.subTest(byte_pos=byte_pos, bit_pos=bit_pos):
          encoded = self.codec.encode(data)
          decoded = self.codec.decode(encoded)
          self.assertEqual(decoded, data, f"Single bit flip at byte {byte_pos}, bit {bit_pos} should be preserved")
  
  def test_type_validation(self):
    """Test that type errors are raised appropriately"""
    with self.assertRaises(TypeError):
      self.codec.encode("string")
    
    with self.assertRaises(TypeError):
      self.codec.encode(123)
    
    with self.assertRaises(TypeError):
      self.codec.decode(b"bytes")
  
  def test_invalid_marker(self):
    """Test that invalid markers are rejected"""
    valid_encoded = self.codec.encode(b'\x00')
    
    # Replace marker with invalid value
    invalid_char = chr(0x4E00)  # CJK char outside marker range
    invalid_encoded = invalid_char + valid_encoded[1:]
    
    with self.assertRaises(ValueError):
      self.codec.decode(invalid_encoded)
  
  def test_encoded_output_is_valid_unicode(self):
    """Verify encoded output contains only valid CJK + marker characters"""
    data = os.urandom(100)
    encoded = self.codec.encode(data)
    
    marker = ord(encoded[0])
    self.assertGreaterEqual(marker, self.codec.marker_base)
    self.assertLessEqual(marker, self.codec.marker_base + self.codec.bit_range)
    
    for char in encoded[1:]:
      code_point = ord(char)
      self.assertGreaterEqual(code_point, self.codec.unicode_range_start)
  
  def test_encoding_efficiency(self):
    """Verify encoding is reasonably efficient (better than base64)"""
    data = os.urandom(1000)
    encoded = self.codec.encode(data)
    
    # Base64 would give us len(data) * 4/3 characters
    base64_equivalent = len(data) * 4 / 3
    
    # We should be significantly better (14 bits per char vs 6 bits per char)
    self.assertLess(len(encoded), base64_equivalent * 0.8, 
                "Encoding should be more efficient than base64")
  
  def test_deterministic_encoding(self):
    """Verify encoding is deterministic"""
    data = os.urandom(100)
    encoded1 = self.codec.encode(data)
    encoded2 = self.codec.encode(data)
    self.assertEqual(encoded1, encoded2, "Encoding should be deterministic")
  
  def test_different_unicode_ranges(self):
    """Test with different Unicode range configurations"""
    # Test with smaller range
    codec_small = Cijak(unicode_range_start=0x4E00, unicode_range_end=0x4FFF)
    data = os.urandom(50)
    encoded = codec_small.encode(data)
    decoded = codec_small.decode(encoded)
    self.assertEqual(decoded, data, "Custom Unicode range should work")
  
  def test_properties_accessible(self):
    """Test that internal properties are accessible"""
    self.assertIsNotNone(self.codec.bit_range)
    self.assertIsNotNone(self.codec.unicode_range_start)
    self.assertIsNotNone(self.codec.marker_base)
    
    # Verify they're read-only by attempting to set them
    with self.assertRaises(AttributeError):
      self.codec.bit_range = 10


if __name__ == '__main__':
  unittest.main(verbosity=2)