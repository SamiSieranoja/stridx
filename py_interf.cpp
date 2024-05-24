
#include <stdio.h>
#include <limits.h>
#include <cstring>
#include <pthread.h>

#include <stdio.h>
#include <iostream>
#include <string>
#include <vector>
#include <cfloat>
#include <cmath>
#include <bits/stdc++.h>

using std::ios;
using std::sort;
using std::string;
using std::vector;

#include <Python.h>
#include <cstring>

#include "stridx.hpp"

extern "C" {

// Define a structure for the custom object
typedef struct {
  PyObject_HEAD int value;
  StrIdx::StringIndex *idx;
} StrIdxObject;

// Method to allocate memory for the object
static PyObject *StrIdxObject_new(PyTypeObject *type, PyObject *args, PyObject *kwds) {
  StrIdxObject *self;

  self = (StrIdxObject *)type->tp_alloc(type, 0);
  if (self != NULL) {
    self->value = 0;
    self->idx = new StrIdx::StringIndex();
  }

  return (PyObject *)self;
}

// Method to deallocate memory for the object
static void StrIdxObject_dealloc(StrIdxObject *self) { Py_TYPE(self)->tp_free((PyObject *)self); }

// Method to set the value of the object
static PyObject *StrIdxObject_set_value(StrIdxObject *self, PyObject *args) {
  int value;

  if (!PyArg_ParseTuple(args, "i", &value)) {
    return NULL;
  }

  self->value = value;

  Py_INCREF(Py_None);
  return Py_None;
}

static PyObject *StrIdxObject_add(StrIdxObject *self, PyObject *args) {
  char *value;
  int file_id;
  std::string str;
  if (!PyArg_ParseTuple(args, "si", &value, &file_id)) {
    return NULL;
  }
  str = value;

  printf("char[]*: %s %i\n", value, file_id);
  self->idx->addStrToIndex(str, file_id);
  // self->idx->addStrToIndexThreaded(str, file_id);
  Py_INCREF(Py_None);
  return Py_None;
}

static PyObject *StrIdxObject_find(StrIdxObject *self, PyObject *args) {
  char *value;
  std::string str;
  if (!PyArg_ParseTuple(args, "s", &value)) {
    return NULL;
  }
  str = value;

  printf("char*: %s\n", value);
  const std::vector<std::pair<float, int>> &results = self->idx->findSimilar(str, 2);

  int limit = 15;
  int i = 0;

  printf("res=%d\n", results.size());
  if (results.size() < limit) {
    limit = results.size();
  }
  PyObject *pyarr = PyList_New(limit);

  for (const auto &[score,fileId] : results) {
    PyObject *arr2 = PyList_New(2);
    // PyList_SetItem(arr2, 0, Py_BuildValue("i", res.second));
    // PyList_SetItem(arr2, 1, Py_BuildValue("d", res.first));
    PyList_SetItem(arr2, 0, Py_BuildValue("i", fileId));
    PyList_SetItem(arr2, 1, Py_BuildValue("d", score));
    PyList_SetItem(pyarr, i, arr2);
    i++;
    if (i >= limit) {
      break;
    }
  }

  // Py_INCREF(Py_None);
  return pyarr;
}

// Method to get the value of the object
static PyObject *StrIdxObject_get_value(StrIdxObject *self) {
  return PyLong_FromLong(self->value);
}

// Define methods of the class
static PyMethodDef StrIdxObject_methods[] = {
    {"set_value", (PyCFunction)StrIdxObject_set_value, METH_VARARGS,
     "Set the value of the object"},
    {"add", (PyCFunction)StrIdxObject_add, METH_VARARGS, "Set the value of the object"},
    {"find", (PyCFunction)StrIdxObject_find, METH_VARARGS, "Find similar strings"},
    {"get_value", (PyCFunction)StrIdxObject_get_value, METH_NOARGS, "Get the value of the object"},
    {NULL} /* Sentinel */
};

// Define the type object for the class
static PyTypeObject StrIdxType = {
    PyVarObject_HEAD_INIT(NULL, 0).tp_name = "stridx.StrIdx",
    .tp_basicsize = sizeof(StrIdxObject),
    .tp_dealloc = (destructor)StrIdxObject_dealloc,
    .tp_doc = PyDoc_STR("Fuzzy string index"),
    .tp_methods = StrIdxObject_methods,
    .tp_new = StrIdxObject_new,
    // .tp_repr = (reprfunc)myobj_repr,
};

// PyVarObject_HEAD_INIT(NULL, 0)
// .tp_name = "stridx.StrIdx",
// .tp_doc = "StrIdx class",
// .tp_basicsize = sizeof(StrIdxObject),
// .tp_itemsize = 0,
// .tp_flags = Py_TPFLAGS_DEFAULT,
// .tp_new = StrIdxObject_new,
// .tp_dealloc = (destructor)StrIdxObject_dealloc,
// .tp_methods = StrIdxObject_methods,
// };


// Define python accessible methods
static PyMethodDef StrIdxMethods[] = {
    {NULL, NULL, 0, NULL}};

static struct PyModuleDef moduledef = {
    PyModuleDef_HEAD_INIT, "stridx", NULL, -1, StrIdxMethods, NULL, NULL, NULL, NULL};

PyMODINIT_FUNC PyInit_stridx(void) {
  PyObject *m;
  m = PyModule_Create(&moduledef);

  // Initialize the type object
  if (PyType_Ready(&StrIdxType) < 0) {
    return NULL;
  }

  Py_INCREF(&StrIdxType);
  if (PyModule_AddObject(m, "StringIndex", (PyObject *)&StrIdxType) < 0) {
    Py_DECREF(&StrIdxType);
    Py_DECREF(m);
    return NULL;
  }

  if (!m) {
    return NULL;
  }
  return m;
}
} // END extern "C"
