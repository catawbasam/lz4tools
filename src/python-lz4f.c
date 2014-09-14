/*
 * Copyright (c) 2012-2014, Christopher Jackson
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. Neither the name of Christopher Jackson nor the names of its contributors 
 *    may be used to endorse or promote products derived from this software 
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <Python.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include "lz4.h"
#include "lz4hc.h"
#include "lz4frame.h"
#include "python-lz4f.h"
#include "structmember.h"


static int LZ4S_GetBlockSize_FromBlockId (int id) { return (1 << (8 + (2 * id))); }

/* Compression methods */ 

static PyObject *py_lz4_createCompressionContext(PyObject *self, PyObject *args) {
    PyObject *result;
    LZ4F_compressionContext_t cCtx;
    size_t err;

    (void)self;
    (void)args;
   
    err = LZ4F_createCompressionContext(&cCtx, LZ4F_VERSION);
    CHECK(LZ4F_isError(err), "Allocation failed (error %i)", (int)err);
    result = PyCObject_FromVoidPtr(cCtx, NULL/*LZ4F_freeDecompressionContext*/);

    return result;
_output_error:
    return Py_None; 
}

static PyObject *py_lz4_freeCompressionContext(PyObject *self, PyObject *args) {
    PyObject *py_cCtx;
    LZ4F_compressionContext_t cCtx;

    (void)self;
    if (!PyArg_ParseTuple(args, "O", &py_cCtx)) {
        return NULL;
    }
    
    cCtx = (LZ4F_compressionContext_t)PyCObject_AsVoidPtr(py_cCtx);
    LZ4F_freeCompressionContext(cCtx);

    return Py_None; 
}


/* Decompression methods */ 
static PyObject *py_lz4_createDecompressionContext(PyObject *self, PyObject *args) {
    PyObject *result;
    LZ4F_decompressionContext_t dCtx;
    size_t err;

    (void)self;
    (void)args;
   
    err = LZ4F_createDecompressionContext(&dCtx, LZ4F_VERSION);
    CHECK(LZ4F_isError(err), "Allocation failed (error %i)", (int)err);
    result = PyCObject_FromVoidPtr(dCtx, NULL/*LZ4F_freeDecompressionContext*/);

    return result;
_output_error:
    return Py_None; 
}

static PyObject *py_lz4_freeDecompressionContext(PyObject *self, PyObject *args) {
    PyObject *py_dCtx;
    LZ4F_compressionContext_t dCtx;

    (void)self;
    if (!PyArg_ParseTuple(args, "O", &py_dCtx)) {
        return NULL;
    }
    
    dCtx = (LZ4F_decompressionContext_t)PyCObject_AsVoidPtr(py_dCtx);
    LZ4F_freeDecompressionContext(dCtx);

    return Py_None; 
}

static PyObject *py_lz4f_getFrameInfo(PyObject *self, PyObject *args) {
    PyObject *result;
    PyObject *py_dCtx;
    LZ4F_decompressionContext_t dCtx;
    LZ4F_frameInfo_t frameInfoHold = { 0 };
    LZ4F_frameInfo_t *frameInfo;
    const char *source;
    size_t source_size;
    size_t err;

    (void)self;
    if (!PyArg_ParseTuple(args, "s#O", &source, &source_size, &py_dCtx)) {
        return NULL;
    }
    
    frameInfo = &frameInfoHold;
    dCtx = (LZ4F_decompressionContext_t)PyCObject_AsVoidPtr(py_dCtx);

    err = LZ4F_getFrameInfo(dCtx, frameInfo, (unsigned char*)source, &source_size);
    CHECK(LZ4F_isError(err), "Failed getting frameInfo. (error %i)", (int)err);

    frameInfoHold = *frameInfo;
    result = PyLong_FromSize_t(frameInfoHold.blockSizeID);

    return result;
_output_error:
    return Py_None; 
}

static PyObject *pass_lz4f_decompress(PyObject *self, PyObject *args, PyObject *keywds) {
    PyObject *result = Py_None;
    PyObject *py_dCtx;
    LZ4F_decompressionContext_t dCtx;
    const char *source;
    size_t source_size;
    size_t dest_size;
    size_t err;
    unsigned int blkID=7;
    static char *kwlist[] = {"source", "dCtx", "blkID"};

    (void)self;
    if (!PyArg_ParseTupleAndKeywords(args, keywds, "s#O|i", kwlist, &source, 
                                     &source_size, &py_dCtx, &blkID)) {
        return NULL;
    }

    dest_size = LZ4S_GetBlockSize_FromBlockId(blkID);
    dCtx = (LZ4F_decompressionContext_t)PyCObject_AsVoidPtr(py_dCtx);
    
    char* dest = (char*)malloc(dest_size);
    err = LZ4F_decompress(dCtx, dest, &dest_size, source, &source_size, NULL);
    CHECK(LZ4F_isError(err), "Failed getting frameInfo. (error %i)", (int)err);
    //fprintf(stdout, "Dest_size: %i  Error Code:%i \n", dest_size, err);
    result = PyBytes_FromStringAndSize(dest, dest_size);
    free(dest);

    return result;
_output_error:
    return Py_None; 
}

static PyMethodDef Lz4Methods[] = {
    {"decompressFrame",  (PyCFunction)pass_lz4f_decompress, METH_VARARGS | METH_KEYWORDS, UNCOMPRESS_DOCSTRING},
    {"getFrameInfo", py_lz4f_getFrameInfo, METH_VARARGS, NULL},
    {"createCompContext", py_lz4_createCompressionContext, METH_VARARGS, NULL},
    {"freeCompContext", py_lz4_freeCompressionContext, METH_VARARGS, NULL},
    {"createDecompContext", py_lz4_createDecompressionContext, METH_VARARGS, NULL},
    {"freeDecompContext", py_lz4_freeDecompressionContext, METH_VARARGS, NULL},
    {NULL, NULL, 0, NULL}
};

struct module_state {
    PyObject *error;
};

#if PY_MAJOR_VERSION >= 3
#define GETSTATE(m) ((struct module_state*)PyModule_GetState(m))
#else
#define GETSTATE(m) (&_state)
static struct module_state _state;
#endif

#if PY_MAJOR_VERSION >= 3

static int myextension_traverse(PyObject *m, visitproc visit, void *arg) {
    Py_VISIT(GETSTATE(m)->error);
    return 0;
}

static int myextension_clear(PyObject *m) {
    Py_CLEAR(GETSTATE(m)->error);
    return 0;
}


static struct PyModuleDef moduledef = {
        PyModuleDef_HEAD_INIT,
        "lz4f",
        NULL,
        sizeof(struct module_state),
        Lz4fMethods,
        NULL,
        myextension_traverse,
        myextension_clear,
        NULL
};

#define INITERROR return NULL
PyObject *PyInit_lz4f(void)

#else
#define INITERROR return
void initlz4f(void)

#endif
{
#if PY_MAJOR_VERSION >= 3
    PyObject *module = PyModule_Create(&moduledef);
#else
    PyObject *module = Py_InitModule("lz4f", Lz4Methods);
#endif
    struct module_state *state = NULL;

    if (module == NULL) {
        INITERROR;
    }
    state = GETSTATE(module);

    state->error = PyErr_NewException("lz4.Error", NULL, NULL);
    if (state->error == NULL) {
        Py_DECREF(module);
        INITERROR;
    }

    PyModule_AddStringConstant(module, "VERSION", VERSION);
    PyModule_AddStringConstant(module, "__version__", VERSION);
    PyModule_AddStringConstant(module, "LZ4_VERSION", LZ4_VERSION);

#if PY_MAJOR_VERSION >= 3
    return module;
#endif
}
