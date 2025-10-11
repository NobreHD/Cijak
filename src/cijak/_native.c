#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include <math.h>

typedef struct {
  PyObject_HEAD
  int unicode_range_start;
  int bit_range;
  int marker_base;
  int _max_code;
  unsigned int _mask;
} CijakObject;

static void
Cijak_dealloc(CijakObject *self)
{
  Py_TYPE(self)->tp_free((PyObject *) self);
}

static PyObject *
Cijak_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
  CijakObject *self;
  self = (CijakObject *) type->tp_alloc(type, 0);
  return (PyObject *) self;
}

static int
Cijak_init(CijakObject *self, PyObject *args, PyObject *kwds)
{
  static char *kwlist[] = {"unicode_range_start", "unicode_range_end", "marker_base", NULL};
  int unicode_range_start = 0x4E00;
  int unicode_range_end = 0x9FFF;
  int marker_base = 0x31C0;

  if (!PyArg_ParseTupleAndKeywords(args, kwds, "|iii", kwlist,
                   &unicode_range_start, &unicode_range_end, &marker_base))
    return -1;

  if (unicode_range_start >= unicode_range_end) {
    PyErr_SetString(PyExc_ValueError, "Unicode range start must be less than end.");
    return -1;
  }

  self->unicode_range_start = unicode_range_start;
  self->bit_range = (int)floor(log2(unicode_range_end - unicode_range_start + 1));
  
  if (self->bit_range < 1 || self->bit_range > 16) {
    PyErr_SetString(PyExc_ValueError, "Bit range needs to be between 1 and 16");
    return -1;
  }

  self->marker_base = marker_base;
  self->_max_code = unicode_range_end - unicode_range_start;
  self->_mask = (1U << self->bit_range) - 1;

  return 0;
}

static PyObject *
Cijak_encode(CijakObject *self, PyObject *args)
{
  Py_buffer buffer;
  PyObject *result = NULL;
  
  if (!PyArg_ParseTuple(args, "y*", &buffer))
    return NULL;

  if (buffer.len == 0) {
    PyBuffer_Release(&buffer);
    return PyUnicode_FromString("");
  }

  const unsigned char *data = (const unsigned char *)buffer.buf;
  Py_ssize_t data_len = buffer.len;

  // Estimate output size
  Py_ssize_t est_size = (data_len * 8) / self->bit_range + 2;
  Py_UCS4 *out_buf = PyMem_Malloc(est_size * sizeof(Py_UCS4));
  if (!out_buf) {
    PyBuffer_Release(&buffer);
    return PyErr_NoMemory();
  }

  unsigned int bit_buffer = 0;
  int bit_count = 0;
  Py_ssize_t out_idx = 1; // Reserve position 0 for marker

  for (Py_ssize_t i = 0; i < data_len; i++) {
    bit_buffer = (bit_buffer << 8) | data[i];
    bit_count += 8;

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

  // Add marker at the beginning
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
  
  if (!PyArg_ParseTuple(args, "U", &unicode_str))
    return NULL;

  Py_ssize_t str_len = PyUnicode_GET_LENGTH(unicode_str);
  
  if (str_len < 2) {
    return PyBytes_FromStringAndSize("", 0);
  }

  int kind = PyUnicode_KIND(unicode_str);
  void *data = PyUnicode_DATA(unicode_str);
  
  Py_UCS4 marker = PyUnicode_READ(kind, data, 0);
  int padding = marker - self->marker_base;

  if (padding < 0 || padding > self->bit_range) {
    PyErr_SetString(PyExc_ValueError, "Invalid marker");
    return NULL;
  }

  Py_ssize_t num_vals = str_len - 1;
  Py_ssize_t total_bits = num_vals * self->bit_range - padding;
  Py_ssize_t out_size = total_bits / 8;
  
  unsigned char *out_buf = PyMem_Malloc(out_size);
  if (!out_buf) {
    return PyErr_NoMemory();
  }

  unsigned int bit_buffer = 0;
  int bit_count = 0;
  Py_ssize_t out_idx = 0;
  Py_ssize_t bits_left = total_bits;

  for (Py_ssize_t i = 1; i < str_len; i++) {
    Py_UCS4 ch = PyUnicode_READ(kind, data, i);
    unsigned int val = (ch - self->unicode_range_start) & self->_mask;
    
    bit_buffer = (bit_buffer << self->bit_range) | val;
    bit_count += self->bit_range;

    while (bit_count >= 8 && bits_left >= 8) {
      bit_count -= 8;
      bits_left -= 8;
      out_buf[out_idx++] = (bit_buffer >> bit_count) & 0xFF;
    }
  }

  PyObject *result = PyBytes_FromStringAndSize((char *)out_buf, out_idx);
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
  {"bit_range", (getter) Cijak_get_bit_range, NULL,
   "Number of bits per encoded character", NULL},
  {"unicode_range_start", (getter) Cijak_get_unicode_range_start, NULL,
   "Start of Unicode range used for encoding", NULL},
  {"marker_base", (getter) Cijak_get_marker_base, NULL,
   "Base value for padding marker", NULL},
  {NULL}
};

static PyMethodDef Cijak_methods[] = {
  {"encode", (PyCFunction) Cijak_encode, METH_VARARGS,
   "Encode bytes to Unicode string"},
  {"decode", (PyCFunction) Cijak_decode, METH_VARARGS,
   "Decode Unicode string to bytes"},
  {NULL}
};

static PyTypeObject CijakType = {
  PyVarObject_HEAD_INIT(NULL, 0)
  .tp_name = "cijak.Cijak",
  .tp_doc = "Cijak encoding/decoding class",
  .tp_basicsize = sizeof(CijakObject),
  .tp_itemsize = 0,
  .tp_flags = Py_TPFLAGS_DEFAULT,
  .tp_new = Cijak_new,
  .tp_init = (initproc) Cijak_init,
  .tp_dealloc = (destructor) Cijak_dealloc,
  .tp_methods = Cijak_methods,
};

static PyModuleDef cijakmodule = {
  PyModuleDef_HEAD_INIT,
  .m_name = "_native",  // Changed from "cijak"
  .m_doc = "Fast Cijak encoding/decoding module",
  .m_size = -1,
};

PyMODINIT_FUNC
PyInit__native(void)
{
  PyObject *m;
  
  if (PyType_Ready(&CijakType) < 0)
    return NULL;

  m = PyModule_Create(&cijakmodule);
  if (m == NULL)
    return NULL;

  Py_INCREF(&CijakType);
  if (PyModule_AddObject(m, "Cijak", (PyObject *) &CijakType) < 0) {
    Py_DECREF(&CijakType);
    Py_DECREF(m);
    return NULL;
  }

  return m;
}