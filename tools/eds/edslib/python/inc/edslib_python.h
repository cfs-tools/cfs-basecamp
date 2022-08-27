/*
 * LEW-19710-1, CCSDS SOIS Electronic Data Sheet Implementation
 * 
 * Copyright (c) 2020 United States Government as represented by
 * the Administrator of the National Aeronautics and Space Administration.
 * All Rights Reserved.
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *    http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


/**
 * \file     edslib_python.h
 * \ingroup  python
 * \author   joseph.p.hickey@nasa.gov
 *
 * API definitions for EdsLib-Python binding library
 */

#ifndef _EDSLIB_PYTHON_H_
#define _EDSLIB_PYTHON_H_


#include <Python.h>
#include <edslib_api_types.h>
#include <edslib_id.h>

/**
 * Documentation string for this Python module
 *
 * This is provided for applications that supply their own custom init routine.
 */
#define EDSLIB_PYTHON_DOC            "Module which provides an interface to the EDS Runtime Library."

/**
 * Base Name of the Python module
 *
 * Applications should use this name if registering a custom inittab.
 * It needs to be consistent because all module-supplied types use this
 * as the base name, which carries through to the respective "repr()"
 * implementation and other user-visible items.
 */
#define EDSLIB_PYTHON_MODULE_NAME    "EdsLib"

/**
 * Get the name of a Python entity
 *
 * This macro converts an unqualified name to a qualified name,
 * by adding a prefix of EDSLIB_PYTHON_MODULE_NAME.  It is used
 * by all types to keep the naming consistent.
 */
#define EDSLIB_PYTHON_ENTITY_NAME(x)   EDSLIB_PYTHON_MODULE_NAME "." x


/**
 * Main Initializer function that sets up a newly-minted module object
 *
 * This calls PyType_Ready() for all the Python type objects defined in
 * this module, and adds the required members to the module, and returns
 * the new module to the caller.
 *
 * This is almost (but not quite) usable as a Python module init function.
 * Depending on the environment, additional members may need to be added,
 * and the API/name might need to be adjusted (Python2 and Python3 have
 * different naming and calling conventions).  So it is expected that
 * an additional wrapper around this will be added to accommodate this.
 */
PyObject* EdsLib_Python_CreateModule(void);

/**
 * Creates a Python EDS Database object from a C Database Object
 *
 * This would typically be used when the DB object is statically linked
 * into the C environment, and should be exposed to Python as a built-in.
 */
PyObject *EdsLib_Python_Database_CreateFromStaticDB(const char *Name, const EdsLib_DatabaseObject_t *GD);

/**
 * Gets the C EDS Database object from a Python EDS Database Object
 *
 * This should be used when calling the EdsLib API from Python.
 */
const EdsLib_DatabaseObject_t *EdsLib_Python_Database_GetDB(PyObject *obj);

/**
 * Gets an entry from a Python EDS Database object using a C EdsLib_Id_t value
 *
 * This is a convenience wrapper that performs type checking and conversions
 */
PyTypeObject *EdsLib_Python_DatabaseEntry_GetFromEdsId(PyObject *dbobj, EdsLib_Id_t EdsId);

/**
 * Instantiate a Python-EDS object from an arbitrary C structure.
 *
 * The returned object will be a copy of the original C structure.  As such,
 * modifications to the Python object will not be visible in the original.
 */
PyObject *EdsLib_Python_ObjectNewCopy(PyTypeObject *objtype, const void *ptr, Py_ssize_t size);

/**
 * Instantiate a Python-EDS object from an arbitrary C structure.
 *
 * The returned object will be refer to the same C structure.  As such,
 * modifications to the Python object will be visible in the original.
 */
PyObject *EdsLib_Python_ObjectFromPtr(PyTypeObject *objtype, void *ptr, Py_ssize_t size);

/**
 * Instantiate a read-only Python-EDS object from an arbitrary C structure.
 *
 * The returned object will be refer to the same C structure, and it is
 * marked as a read-only object.  This mainly affects usage of the "buffer
 * protocol" from Python.  An attempt to get a writable buffer using the
 * buffer protocol API will fail.
 */
PyObject *EdsLib_Python_ObjectFromConstPtr(PyTypeObject *objtype, const void *ptr, Py_ssize_t size);

/**
 * Instantiate a Python-EDS object from another Python object via the buffer protocol.
 *
 * The returned EDS object will be refer to the same buffer.  Whether
 * the object is read-write or read-only depends on the "readonly" parameter.
 */
PyObject *EdsLib_Python_ObjectFromBuffer(PyTypeObject *objtype, PyObject *bufobj, int readonly);

/**
 * Gets a direct pointer to the buffer memory storage of a Python EDS object
 *
 * This should be used sparingly as it does not increment the reference count
 * of the object.  The pointer must _not_ be kept across calls to the Python
 * interpreter (i.e. use it and then lose it) as there is no protection
 * against the object being destroyed should its refcount become zero.
 */
const void *EdsLib_Python_ObjectPeek(PyObject *obj);

/**
 * Gets the maximum size of an object of the given type.
 *
 * For EDS objects which are not a fixed size this obtains the
 * maximum size for allocation purposes.  If the specified type
 * is the base type for other types, then this wil return
 * the size of the largest possible derivative type.
 */
Py_ssize_t EdsLib_Python_DatabaseEntry_GetMaxSize(PyTypeObject* objtype);

/**
 * Create an EDS dynamic array from a C array.
 *
 * A Dynamic Array is simply an array created at runtime, where each
 * element is an EDS object.  This is functionally similar to the
 * arrays defined in EDS however they can be of any size, whereas
 * EDS-defined arrays have a static size.
 *
 * The returned Python EDS object uses the same memory as its
 * buffer storage.  The buffer will be "read-write" so modifications
 * are possible from Python and these will be visible from C.
 */
PyObject *EdsLib_Python_DynamicArray_FromPtrAndSize(PyTypeObject *objtype, void *ptr, Py_ssize_t nelem, Py_ssize_t elemsz);

/**
 * Create an EDS dynamic array from a C array.
 *
 * A Dynamic Array is simply an array created at runtime, where each
 * element is an EDS object.  This is functionally similar to the
 * arrays defined in EDS however they can be of any size, whereas
 * EDS-defined arrays have a static size.
 *
 * The returned Python EDS object uses the same memory as its
 * buffer storage.  The buffer will be "read-only" so modifications
 * are not possible from Python.  Attempts to get a writable buffer
 * using the buffer protocol will fail.
 */
PyObject *EdsLib_Python_DynamicArray_FromConstPtrAndSize(PyTypeObject *objtype, const void *ptr, Py_ssize_t nelem, Py_ssize_t elemsz);


#endif  /* _EDSLIB_PYTHON_H_ */

