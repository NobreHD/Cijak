#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include <math.h>

typedef struct {
  PyObject_HEAD
  int unicode_range_start;
  int bit_range;
  int marker_base;
  unsigned int _mask;
} CijakObject;

#define DEFAULT_UNICODE_START 0x4E00
#define DEFAULT_UNICODE_END   0x9FFF
#define DEFAULT_MARKER_BASE   0x31C0
#define MIN_BIT_RANGE         1
#define MAX_BIT_RANGE         16
#define BITS_PER_BYTE         8

static void Cijak_dealloc(CijakObject *self);
static PyObject *Cijak_new(PyTypeObject *type, PyObject *args, PyObject *kwds);
static int Cijak_init(CijakObject *self, PyObject *args, PyObject *kwds);
static PyObject *Cijak_encode(CijakObject *self, PyObject *args);
static PyObject *Cijak_decode(CijakObject *self, PyObject *args);

static void
Cijak_dealloc(CijakObject *self)
{
  Py_TYPE(self)->tp_free((PyObject *) self);
}

static PyObject *
Cijak_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
  CijakObject *self = (CijakObject *) type->tp_alloc(type, 0);
  return (PyObject *) self;
}

static int
Cijak_init(CijakObject *self, PyObject *args, PyObject *kwds)
{
  static char *kwlist[] = {"unicode_range_start", "unicode_range_end", "marker_base", NULL};
  int unicode_range_start = DEFAULT_UNICODE_START;
  int unicode_range_end = DEFAULT_UNICODE_END;
  int marker_base = DEFAULT_MARKER_BASE;

  if (!PyArg_ParseTupleAndKeywords(args, kwds, "|iii", kwlist,
                                    &unicode_range_start, 
                                    &unicode_range_end, 
                                    &marker_base)) {
    return -1;
  }

  if (unicode_range_start >= unicode_range_end) {
      PyErr_SetString(PyExc_ValueError, 
                      "Unicode range start must be less than end.");
      return -1;
  }

  self->unicode_range_start = unicode_range_start;
  self->marker_base = marker_base;

  // Calculate the number of bits required to represent the Unicode range.
  // We want the largest power of 2 that fits within the given range.
  // floor(log2(range_size)) gives us the exponent 'n'.
  self->bit_range = (int)floor(log2(unicode_range_end - unicode_range_start + 1));
  
  if (self->bit_range < MIN_BIT_RANGE || self->bit_range > MAX_BIT_RANGE) {
      PyErr_Format(PyExc_ValueError, 
                    "Bit range must be between %d and %d, got %d",
                    MIN_BIT_RANGE, MAX_BIT_RANGE, self->bit_range);
      return -1;
  }

  self->_mask = (1U << self->bit_range) - 1;

  return 0;
}

static PyObject *
Cijak_encode(CijakObject *self, PyObject *args)
{
  Py_buffer buffer;
  PyObject *result = NULL;
  Py_UCS4 *out_buf = NULL;
  
  if (!PyArg_ParseTuple(args, "y*", &buffer)) {
    return NULL;
  }

  if (buffer.len == 0) {
    PyBuffer_Release(&buffer);
    return PyUnicode_FromString("");
  }

  const unsigned char *data = (const unsigned char *)buffer.buf;
  Py_ssize_t data_len = buffer.len;

  // Estimate output buffer size. Over-estimate slightly to be safe.
  // (data_len * 8 bits/byte) / (bit_range bits/char)
  // +1 for potential partial chunk, +1 for the marker character.
  Py_ssize_t est_size = (data_len * BITS_PER_BYTE) / self->bit_range + 2;
  out_buf = PyMem_Malloc(est_size * sizeof(Py_UCS4));
  if (!out_buf) {
    PyBuffer_Release(&buffer);
    return PyErr_NoMemory();
  }

  unsigned int bit_buffer = 0;
  int bit_count = 0;
  Py_ssize_t out_idx = 1;

  for (Py_ssize_t i = 0; i < data_len; i++) {
    bit_buffer = (bit_buffer << BITS_PER_BYTE) | data[i];
    bit_count += BITS_PER_BYTE;

    while (bit_count >= self->bit_range) {
      bit_count -= self->bit_range;
      unsigned int val = (bit_buffer >> bit_count) & self->_mask;
      out_buf[out_idx++] = self->unicode_range_start + val;
    }
  }

  int pad_bits = 0;
  if (bit_count > 0) {
    pad_bits = self->bit_range - bit_count;
    unsigned int val = (bit_buffer << pad_bits) & self->_mask;
    out_buf[out_idx++] = self->unicode_range_start + val;
  }

  out_buf[0] = self->marker_base + pad_bits;

  result = PyUnicode_FromKindAndData(PyUnicode_4BYTE_KIND, out_buf, out_idx);
  
  PyMem_Free(out_buf);
  PyBuffer_Release(&buffer);
  return result;
}

static PyObject *
Cijak_decode(CijakObject *self, PyObject *args)
{
  PyObject *unicode_str;
  unsigned char *out_buf = NULL;
  PyObject *result = NULL;
  
  if (!PyArg_ParseTuple(args, "U", &unicode_str)) {
    return NULL;
  }

  Py_ssize_t str_len = PyUnicode_GET_LENGTH(unicode_str);
  
  if (str_len < 2) {
    return PyBytes_FromStringAndSize("", 0);
  }

  int kind = PyUnicode_KIND(unicode_str);
  void *data = PyUnicode_DATA(unicode_str);
  
  Py_UCS4 marker = PyUnicode_READ(kind, data, 0);
  int padding = marker - self->marker_base;

  if (padding < 0 || padding > self->bit_range) {
    PyErr_Format(PyExc_ValueError, 
                  "Invalid padding marker: %d (expected 0-%d)",
                  padding, self->bit_range);
    return NULL;
  }

  Py_ssize_t num_vals = str_len - 1;
  Py_ssize_t total_bits = num_vals * self->bit_range - padding;
  Py_ssize_t out_size = total_bits / BITS_PER_BYTE;
  
  out_buf = PyMem_Malloc(out_size);
  if (!out_buf) {
    return PyErr_NoMemory();
  }

  unsigned int bit_buffer = 0;
  int bit_count = 0;
  Py_ssize_t out_idx = 0;
  Py_ssize_t bits_left = total_bits;

  for (Py_ssize_t i = 1; i < str_len; i++) {
    Py_UCS4 ch = PyUnicode_READ(kind, data, i);

    if (ch < self->unicode_range_start) {
      PyErr_Format(PyExc_ValueError, "Invalid character 0x%X in input string", ch);
      PyMem_Free(out_buf);
      return NULL;
    }

    unsigned int val = (ch - self->unicode_range_start);

    if (val > self->_mask) {
      PyErr_Format(PyExc_ValueError, "Character 0x%X out of range for encoding", ch);
      PyMem_Free(out_buf);
      return NULL;
    }
    
    bit_buffer = (bit_buffer << self->bit_range) | val;
    bit_count += self->bit_range;

    while (bit_count >= BITS_PER_BYTE && bits_left >= BITS_PER_BYTE) {
      bit_count -= BITS_PER_BYTE;
      bits_left -= BITS_PER_BYTE;
      out_buf[out_idx++] = (bit_buffer >> bit_count) & 0xFF;
    }
  }

  result = PyBytes_FromStringAndSize((char *)out_buf, out_idx);
  PyMem_Free(out_buf);
  return result;
}

static PyObject *
Cijak_get_bit_range(CijakObject *self, void *closure)
{
  return PyLong_FromLong(self->bit_range);
}

static PyObject *
Cijak_get_unicode_range_start(CijakObject *self, void *closure)
{
  return PyLong_FromLong(self->unicode_range_start);
}

static PyObject *
Cijak_get_marker_base(CijakObject *self, void *closure)
{
  return PyLong_FromLong(self->marker_base);
}

static PyGetSetDef Cijak_getsetters[] = {
  {"bit_range", 
    (getter) Cijak_get_bit_range, NULL,
    "The number of bits (n) represented by each encoded character.", NULL},
  {"unicode_range_start", 
    (getter) Cijak_get_unicode_range_start, NULL,
    "Start of Unicode range used for encoding", NULL},
  {"marker_base", 
    (getter) Cijak_get_marker_base, NULL,
    "The base codepoint for the padding marker.", NULL},
  {NULL}
};

static PyMethodDef Cijak_methods[] = {
  {"encode", (PyCFunction) Cijak_encode, METH_VARARGS,
    "encode(self, data: bytes) -> str\n"
    "Encodes a bytes object into a Unicode string.\n"
    "Args:"
    "    data (bytes): The raw byte data to encode.\n"
    "Returns:"
    "    str: The resulting Unicode string, with the first character being a padding marker."},
  {"decode", (PyCFunction) Cijak_decode, METH_VARARGS,
    "decode(self, s: str) -> bytes\n\n"
    "Decodes a Cijak-encoded Unicode string back into bytes.\n"
    "Args:"
    "    s (str): The Unicode string to decode. Must begin with a valid padding marker.\n"
    "Returns:"
    "    bytes: The original raw byte data.\n"
    "Raises:"
    "    ValueError: If the string contains an invalid padding marker or characters outside the object's encoding range."},
  {NULL}
};

static PyTypeObject CijakType = {
  PyVarObject_HEAD_INIT(NULL, 0)
  .tp_name = "cijak.Cijak",
  .tp_doc = 
    "Cijak(unicode_range_start=0x4E00, unicode_range_end=0x9FFF, marker_base=0x31C0)\n"
    "Creates an encoder/decoder object that packs byte data into a range of Unicode characters.\n"
    "Args:"
    "    unicode_range_start (int): The first Unicode codepoint to use for encoding. Default is 0x4E00 (Start of CJK Unified Ideographs)."
    "    unicode_range_end (int): The last *potential* Unicode codepoint. The actual range used will be a power of 2. Default is 0x9FFF."
    "    marker_base (int): The first codepoint in the range used for the padding marker. Default is 0x31C0.\n"
    "Raises:"
    "    ValueError: If the Unicode range is invalid or results in an unsupported bit_range.",
  .tp_basicsize = sizeof(CijakObject),
  .tp_itemsize = 0,
  .tp_flags = Py_TPFLAGS_DEFAULT,
  .tp_new = Cijak_new,
  .tp_init = (initproc) Cijak_init,
  .tp_dealloc = (destructor) Cijak_dealloc,
  .tp_methods = Cijak_methods,
  .tp_getset = Cijak_getsetters,
};

static PyModuleDef cijakmodule = {
  PyModuleDef_HEAD_INIT,
  .m_name = "_native", 
  .m_doc = "Fast Cijak encoding/decoding module",
  .m_size = -1,
};

PyMODINIT_FUNC
PyInit__native(void)
{
  PyObject *m;
  
  if (PyType_Ready(&CijakType) < 0) {
    return NULL;
  }

  m = PyModule_Create(&cijakmodule);
  if (m == NULL) {
    return NULL;
  }

  Py_INCREF(&CijakType);
  if (PyModule_AddObject(m, "Cijak", (PyObject *) &CijakType) < 0) {
    Py_DECREF(&CijakType);
    Py_DECREF(m);
    return NULL;
  }

  return m;
}