__version__: str
__all__: list[str]

class Cijak:
    """
    Creates an encoder/decoder object that packs byte data into a range of Unicode characters.
    
    Args:
        unicode_range_start (int): The first Unicode codepoint to use for encoding. Default is 0x4E00 (Start of CJK Unified Ideographs).
        unicode_range_end (int): The last *potential* Unicode codepoint. The actual range used will be a power of 2. Default is 0x9FFF.
        marker_base (int): The first codepoint in the range used for the padding marker. Default is 0x31C0.
        
    Raises:
        ValueError: If the Unicode range is invalid or results in an unsupported bit_range.
    """

    def __init__(
        self,
        unicode_range_start: int = 0x4E00,
        unicode_range_end: int = 0x9FFF,
        marker_base: int = 0x31C0,
    ) -> None: ...

    def encode(self, data: bytes) -> str:
        """
        Encodes a bytes object into a Unicode string.
        
        Args:
            data (bytes): The raw byte data to encode.
            
        Returns:
            str: The resulting Unicode string, with the first character being a padding marker.
        """
        ...

    def decode(self, s: str) -> bytes:
        """
        Decodes a Cijak-encoded Unicode string back into bytes.
        
        Args:
            s (str): The Unicode string to decode. Must begin with a valid padding marker.
            
        Returns:
            bytes: The original raw byte data.
            
        Raises:
            ValueError: If the string contains an invalid padding marker or characters outside the object's encoding range.
        """
        ...

    @property
    def bit_range(self) -> int: ...

    @property
    def unicode_range_start(self) -> int: ...

    @property
    def marker_base(self) -> int: ...