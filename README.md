# Cijak
Cijak is a Python library that provides a unique encoding and decoding mechanism using Unicode characters. It takes advantage of the CJK Unicode block to encode up to 14 bits of data per character. The name "Cijak" originates from the CJK Unicode block, where it's possible to encode 14 bits of data (C**1**J**4**K).

## Key Features
- Encoding and Decoding of binary data using Unicode characters;
- Uses the CJK Unicode block to encode data by default, but can be configured to use other Unicode characters.

## Technical Details
- The Unicode range can be customized by adjusting the `unicode_range_start`, `unicode_range_end`, and `marker_base` parameters of the `Cijak` class (note that the range of characters should not contain any control characters to ensure correct functionality);
- Modifying the default Unicode range allows for adjusting the amount of bits that can be encoded per character, with the library automatically calculating the available bits based on the specified range, which should be provided as UTF-16 code units (eg. 0x4E00 for the first CJK character).

## Uses
Cijak provides character-efficient encoding for scenarios where character limits are a concern. It achieves this by using a base16384 alphabet, encoding 14 bits per character, which is more efficient than base64's 6 bits per character. Despite a larger size increase (72% vs. base64's 33%), Cijak's character density makes it valuable for sending short messages on Unicode-supporting platforms with character restrictions, such as social media and messaging apps.

## Installation
```bash
pip install cijak
```

## Examples
To get started with Cijak, you can use the following examples:

```python
from cijak import Cijak
cijak_encoding = Cijak()
data = b'Hello, World!' # You can replace this with any binary data
encoded_data = cijak_encoding.encode(data)
print(encoded_data) # ㇈怙擆羼稠蔦羐漀

decoded_data = cijak_encoding.decode(encoded_data)
print(decoded_data) # b'Hello, World!'
```
