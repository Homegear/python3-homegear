/* Copyright 2013-2018 Homegear GmbH
 *
 * Homegear is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * Homegear is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with Homegear.  If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * In addition, as a special exception, the copyright holders give
 * permission to link the code of portions of this program with the
 * OpenSSL library under certain conditions as described in each
 * individual source file, and distribute linked combinations
 * including the two.
 * You must obey the GNU Lesser General Public License in all respects
 * for all of the code used other than OpenSSL.  If you modify
 * file(s) with this exception, you may extend this exception to your
 * version of the file(s), but you are not obligated to do so.  If you
 * do not wish to do so, delete this exception statement from your
 * version.  If you delete this exception statement from all source
 * files in the program, then also delete it here.
*/

#include <Python.h>
#include "IpcClient.h"
#include "PythonVariableConverter.h"

static std::shared_ptr<IpcClient> _ipcClient;
static PyObject* _eventCallback = nullptr;
static std::mutex _onConnectWaitMutex;
static std::condition_variable _onConnectConditionVariable;

typedef struct
{
    PyObject_HEAD
    std::string* socketPath;
} HomegearObject;

static PyObject* Homegear_call(PyObject* object, PyObject* attrName);
static void Homegear_dealloc(HomegearObject* self);
static int Homegear_init(HomegearObject* self, PyObject* arg);
static PyObject* Homegear_new(PyTypeObject* type, PyObject* arg, PyObject* kw);

static PyMethodDef HomegearMethods[] = {
        { nullptr, nullptr, 0, nullptr }
};

static PyTypeObject HomegearObjectType = {
        PyVarObject_HEAD_INIT(nullptr, 0)
        "homegear.Homegear",  // tp_name (module name, object name)
        sizeof(HomegearObject),       // tp_basicsize
        0,                           // tp_itemsize
        (destructor)Homegear_dealloc, // tp_dealloc
        nullptr,                           // tp_print
        nullptr,                           // tp_getattr
        nullptr,                           // tp_setattr
        nullptr,                           // tp_as_async
        nullptr,                           // tp_repr
        nullptr,                           // tp_as_number
        nullptr,                           // tp_as_sequence
        nullptr,                           // tp_as_mapping
        nullptr,                           // tp_hash
        nullptr,                           // tp_call
        nullptr,                           // tp_str
        Homegear_call,                           // tp_getattro
        nullptr,                           // tp_setattro
        nullptr,                           // tp_as_buffer
        Py_TPFLAGS_DEFAULT,          // tp_flags
        "Class to locally communicate with Homegear.",                           // tp_doc
        nullptr,                           // tp_traverse
        nullptr,                           // tp_clear
        nullptr,                           // tp_richcompare
        0,                           // tp_weaklistoffset
        nullptr,                           // tp_iter
        nullptr,                           // tp_iternext
        HomegearMethods,                     // tp_methods
        nullptr,                           // tp_members
        nullptr,                           // tp_getset
        nullptr,                           // tp_base
        nullptr,                           // tp_dict
        nullptr,                           // tp_descr_get
        nullptr,                           // tp_descr_set
        0,                           // tp_dictoffset
        (initproc)Homegear_init,      // tp_init
        nullptr,                           // tp_alloc
        Homegear_new,                 // tp_new
        nullptr, // tp_free
};

typedef struct
{
    PyObject_HEAD
    std::string* methodName;
} HomegearRpcMethod;

static PyObject* HomegearRpcMethod_call(PyObject* object, PyObject* args, PyObject* kw);
static void HomegearRpcMethod_dealloc(HomegearRpcMethod* self);
static PyObject* HomegearRpcMethod_new(PyTypeObject* type, PyObject* arg, PyObject* kw);

static PyTypeObject HomegearRpcMethodType = {
        PyVarObject_HEAD_INIT(nullptr, 0)
        "homegear.HomegearRpcMethod",  // tp_name (module name, object name)
        sizeof(HomegearRpcMethodType),       // tp_basicsize
        0,                           // tp_itemsize
        (destructor)HomegearRpcMethod_dealloc, // tp_dealloc
        nullptr,                           // tp_print
        nullptr,                           // tp_getattr
        nullptr,                           // tp_setattr
        nullptr,                           // tp_as_async
        nullptr,                           // tp_repr
        nullptr,                           // tp_as_number
        nullptr,                           // tp_as_sequence
        nullptr,                           // tp_as_mapping
        nullptr,                           // tp_hash
        HomegearRpcMethod_call,                           // tp_call
        nullptr,                           // tp_str
        nullptr,                           // tp_getattro
        nullptr,                           // tp_setattro
        nullptr,                           // tp_as_buffer
        Py_TPFLAGS_DEFAULT,          // tp_flags
        nullptr,                           // tp_doc
        nullptr,                           // tp_traverse
        nullptr,                           // tp_clear
        nullptr,                           // tp_richcompare
        0,                           // tp_weaklistoffset
        nullptr,                           // tp_iter
        nullptr,                           // tp_iternext
        nullptr,                     // tp_methods
        nullptr,                           // tp_members
        nullptr,                           // tp_getset
        nullptr,                           // tp_base
        nullptr,                           // tp_dict
        nullptr,                           // tp_descr_get
        nullptr,                           // tp_descr_set
        0,                           // tp_dictoffset
        nullptr,      // tp_init
        nullptr,                           // tp_alloc
        HomegearRpcMethod_new,                 // tp_new
        nullptr, // tp_free
};

static PyObject* HomegearRpcMethod_new(PyTypeObject* type, PyObject* arg, PyObject* kw)
{
    const char* methodName = nullptr;

    switch(PyTuple_Size(arg))
    {
        case 1:
            if(!PyArg_ParseTuple(arg, "s", &methodName)) return nullptr;
            break;
        default:
            return nullptr;
    }

    if(!methodName) return nullptr;

    auto self = (HomegearRpcMethod*)type->tp_alloc(type, 0);
    if(!self) return nullptr;
    //Py_INCREF(self); //valgrind does not complain if we don't do this and the dealloc is only called after setting the object to "None".
    self->methodName = new std::string(methodName);

    return (PyObject*)self;
}

static void HomegearRpcMethod_dealloc(HomegearRpcMethod* self)
{
	if(self->methodName)
	{
		delete self->methodName;
		self->methodName = nullptr;
	}
}

static PyObject* HomegearRpcMethod_call(PyObject* object, PyObject* args, PyObject* kw)
{
    if(!_ipcClient) { Py_RETURN_NONE; }

    if(*(((HomegearRpcMethod*)object)->methodName) == "connected")
    {
        if(_ipcClient && _ipcClient->connected())
        {
            Py_RETURN_TRUE;
        }
        else
        {
            Py_RETURN_FALSE;
        }
    }

    if(!_ipcClient->connected()) Py_RETURN_NONE;

    auto parameters = PythonVariableConverter::getVariable(args);
    auto result = _ipcClient->invoke(*(((HomegearRpcMethod*)object)->methodName), parameters->arrayValue);
    if(result->errorStruct)
    {
        PyErr_SetString(PyExc_Exception, result->structValue->at("faultString")->stringValue.c_str());
        return nullptr;
    }

    return PythonVariableConverter::getPythonVariable(result);
}

static void Homegear_onConnect()
{
    std::unique_lock<std::mutex> waitLock(_onConnectWaitMutex);
    waitLock.unlock();
    _onConnectConditionVariable.notify_all();
}

static void Homegear_broadcastEvent(uint64_t peerId, int32_t channel, std::string& variableName, Ipc::PVariable value)
{
    if(!_eventCallback) return;
    PyGILState_STATE gstate;
    gstate = PyGILState_Ensure();
    PyObject* pythonValue = PythonVariableConverter::getPythonVariable(value);
    if(pythonValue == nullptr) return;
    PyObject* arglist = Py_BuildValue("(KisO)", (unsigned long long)peerId, (int)channel, variableName.c_str(), pythonValue);
    if(arglist == nullptr)
    {
        Py_DECREF(pythonValue);
        PyGILState_Release(gstate);
        return;
    }
    PyObject* result = PyObject_Call(_eventCallback, arglist, nullptr);
    Py_DECREF(arglist);
    Py_DECREF(pythonValue);
    if (result == nullptr)
    {
        PyGILState_Release(gstate);
        return;
    }
    Py_DECREF(result);
    PyGILState_Release(gstate);
}

static PyObject* Homegear_new(PyTypeObject* type, PyObject* arg, PyObject* kw)
{
    const char* socketPath = nullptr;
    PyObject* tempEventCallback = nullptr;

    switch(PyTuple_Size(arg))
    {
        case 1:
            if(!PyArg_ParseTuple(arg, "s", &socketPath)) return nullptr;
            break;
        case 2:
            if(!PyArg_ParseTuple(arg, "sO:Homegear_new", &socketPath, &tempEventCallback)) return nullptr;
            if (!PyCallable_Check(tempEventCallback))
            {
                PyErr_SetString(PyExc_TypeError, "Parameter eventCallback must be callable.");
                return nullptr;
            }
            Py_XINCREF(tempEventCallback);         /* Add a reference to new callback */
            if(_eventCallback) Py_XDECREF(_eventCallback);  /* Dispose of previous callback */
            _eventCallback = tempEventCallback;       /* Remember new callback */
            break;
        default:
            return nullptr;
    }

    if(!socketPath) return nullptr;

    auto self = (HomegearObject*)type->tp_alloc(type, 0);
    if(!self) return nullptr;
    //Py_INCREF(self); //valgrind does not complain if we don't do this and the dealloc is only called after setting the object to "None".

    self->socketPath = new std::string(socketPath);

    return (PyObject*)self;
}

static int Homegear_init(HomegearObject* self, PyObject* arg)
{
    if(!_ipcClient)
    {
        _ipcClient = std::make_shared<IpcClient>(*(self->socketPath));
        if(_eventCallback) _ipcClient->setBroadcastEvent(std::function<void(uint64_t, int32_t, std::string&, Ipc::PVariable)>(std::bind(&Homegear_broadcastEvent, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4)));
        _ipcClient->setOnConnect(std::function<void(void)>(std::bind(&Homegear_onConnect)));
        _ipcClient->start();
        std::unique_lock<std::mutex> waitLock(_onConnectWaitMutex);
        int64_t startTime = Ipc::HelperFunctions::getTime();
        while (!_onConnectConditionVariable.wait_for(waitLock, std::chrono::milliseconds(2000), [&]
        {
            if(Ipc::HelperFunctions::getTime() - startTime > 2000) return true;
            else return _ipcClient->connected();
        }));
    }

    return 0;
}

static void Homegear_dealloc(HomegearObject* self)
{
	if(self->socketPath)
	{
		delete self->socketPath;
		self->socketPath = nullptr;
	}
}

static PyObject* Homegear_call(PyObject* object, PyObject* attrName)
{
    if(!PyUnicode_Check(attrName))
    {
        PyErr_Format(PyExc_TypeError, "Attribute name must be string, not '%.200s'", attrName->ob_type->tp_name);
        return nullptr;
    }

    Py_ssize_t methodNameSize = 0;
    const char* methodName = PyUnicode_AsUTF8AndSize(attrName, &methodNameSize);
    if(!methodName) return nullptr;

    auto homegearMethodObject = (HomegearRpcMethod*)HomegearRpcMethodType.tp_alloc(&HomegearRpcMethodType, 0);
    if(!homegearMethodObject) return nullptr;
    //Py_INCREF(homegearMethodObject); //valgrind does not complain if we don't do this and the dealloc is only called after setting the object to "None".

    homegearMethodObject->methodName = new std::string(methodName, methodNameSize);

    return (PyObject*)homegearMethodObject;
}

static struct PyModuleDef HomegearModule = {
        PyModuleDef_HEAD_INIT,
        "homegear",   /* name of module */
        nullptr, /* module documentation, may be NULL */
        -1,       /* size of per-interpreter state of the module, or -1 if the module keeps state in global variables. */
        nullptr
};

PyMODINIT_FUNC PyInit_homegear(void)
{
    PyEval_InitThreads(); //Bugfix - Not needed in Python 3.6+, no-op when called for a second time. Must be called from the main thread.

    if(PyType_Ready(&HomegearObjectType) < 0 || PyType_Ready(&HomegearRpcMethodType) < 0) return nullptr;

    PyObject* m = PyModule_Create(&HomegearModule);

    if(!m) return nullptr;

    Py_INCREF(&HomegearObjectType);
    PyModule_AddObject(m, "Homegear", (PyObject*)&HomegearObjectType);

    Py_INCREF(&HomegearRpcMethodType);
    PyModule_AddObject(m, "HomegearRpcMethod", (PyObject*)&HomegearRpcMethodType);

    return m;
}
