/* Copyright 2013-2019 Homegear GmbH
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
#include <unordered_set>

#if PY_MAJOR_VERSION > 3
#error "Python version > 3 is not supported."
#elif PY_MAJOR_VERSION < 3
#error "Python version < 3 is not supported."
#endif

static const std::unordered_set<std::string> kNodeMethods{
    "nodeOutput",
    "setNodeData",
    "setFlowData",
    "setGlobalData",
    "getNodeData",
    "getFlowData",
    "getGlobalData",
    "getNodeCredentials",
    "setNodeCredentials"
};

typedef struct {
  PyObject_HEAD
  std::string *socketPath = nullptr;
  IpcClient *ipcClient = nullptr;
  PyObject *eventCallback = nullptr;
  std::mutex *onConnectWaitMutex = nullptr;
  std::condition_variable *onConnectConditionVariable = nullptr;

// {{{ Variables and methods for use as Node-BLUE node
  std::string *nodeId = nullptr;
  PyObject *nodeInputCallback = nullptr;
// }}}
} HomegearObject;

static PyObject *Homegear_call(PyObject *object, PyObject *attrName);
static void Homegear_dealloc(HomegearObject *self);
static int Homegear_init(HomegearObject *self, PyObject *arg);
static PyObject *Homegear_new(PyTypeObject *type, PyObject *arg, PyObject *kw);

static PyMethodDef HomegearMethods[] = {
    {nullptr, nullptr, 0, nullptr}
};

#if __GNUC__ > 7
static PyTypeObject HomegearObjectType = {
    .ob_base =  PyVarObject_HEAD_INIT(nullptr, 0)
    .tp_name =   "homegear.Homegear",
    .tp_basicsize =  sizeof(HomegearObject),
    .tp_dealloc =  (destructor)Homegear_dealloc,
    .tp_getattro =  Homegear_call,
    .tp_flags =  Py_TPFLAGS_DEFAULT,
    .tp_doc =  "Class to locally communicate with Homegear.",
    .tp_methods =  HomegearMethods,
    .tp_init =  (initproc)Homegear_init,
    .tp_new =  Homegear_new,
};
#else
static PyTypeObject HomegearObjectType = {
        PyVarObject_HEAD_INIT(nullptr, 0)
        "homegear.Homegear",               // tp_name (module name, object name)
        sizeof(HomegearObject),            // tp_basicsize
        0,                                 // tp_itemsize
        (destructor)Homegear_dealloc,      // tp_dealloc
#if PY_MINOR_VERSION >= 8
        0,                                 // tp_vectorcall_offset
#else
        nullptr,                           // tp_print
#endif
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
        Homegear_call,                     // tp_getattro
        nullptr,                           // tp_setattro
        nullptr,                           // tp_as_buffer
        Py_TPFLAGS_DEFAULT,                // tp_flags
        "Class to locally communicate with Homegear.", // tp_doc
        nullptr,                           // tp_traverse
        nullptr,                           // tp_clear
        nullptr,                           // tp_richcompare
        0,                                 // tp_weaklistoffset
        nullptr,                           // tp_iter
        nullptr,                           // tp_iternext
        HomegearMethods,                   // tp_methods
        nullptr,                           // tp_members
        nullptr,                           // tp_getset
        nullptr,                           // tp_base
        nullptr,                           // tp_dict
        nullptr,                           // tp_descr_get
        nullptr,                           // tp_descr_set
        0,                                 // tp_dictoffset
        (initproc)Homegear_init,           // tp_init
        nullptr,                           // tp_alloc
        Homegear_new,                      // tp_new
        nullptr,                           // tp_free
#if PY_MINOR_VERSION >= 8
        nullptr,                           // tp_is_gc
#endif
};
#endif

typedef struct {
  PyObject_HEAD
  std::string *methodName = nullptr;
  std::string *nodeId = nullptr;
  IpcClient *ipcClient = nullptr;
} HomegearRpcMethod;

static PyObject *HomegearRpcMethod_call(PyObject *object, PyObject *args, PyObject *kw);
static void HomegearRpcMethod_dealloc(HomegearRpcMethod *self);
static PyObject *HomegearRpcMethod_new(PyTypeObject *type, PyObject *arg, PyObject *kw);

std::nullptr_t bla;

#if __GNUC__ > 7
static PyTypeObject HomegearRpcMethodType = {
    .ob_base = PyVarObject_HEAD_INIT(nullptr, 0)
    .tp_name = "homegear.HomegearRpcMethod", // (module name, object name)
    .tp_basicsize = sizeof(HomegearRpcMethodType),
    .tp_dealloc = (destructor)HomegearRpcMethod_dealloc,
    .tp_call = HomegearRpcMethod_call,
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_new = HomegearRpcMethod_new
};
#else
static PyTypeObject HomegearRpcMethodType = {
        PyVarObject_HEAD_INIT(nullptr, 0)
        "homegear.HomegearRpcMethod",      // tp_name (module name, object name)
        sizeof(HomegearRpcMethodType),     // tp_basicsize
        0,                                 // tp_itemsize
        (destructor)HomegearRpcMethod_dealloc, // tp_dealloc
#if PY_MINOR_VERSION >= 8
        0,                                 // tp_vectorcall_offset
#else
        nullptr,                           // tp_print
#endif
        nullptr,                           // tp_getattr
        nullptr,                           // tp_setattr
        nullptr,                           // tp_as_async
        nullptr,                           // tp_repr
        nullptr,                           // tp_as_number
        nullptr,                           // tp_as_sequence
        nullptr,                           // tp_as_mapping
        nullptr,                           // tp_hash
        HomegearRpcMethod_call,            // tp_call
        nullptr,                           // tp_str
        nullptr,                           // tp_getattro
        nullptr,                           // tp_setattro
        nullptr,                           // tp_as_buffer
        Py_TPFLAGS_DEFAULT,                // tp_flags
        nullptr,                           // tp_doc
        nullptr,                           // tp_traverse
        nullptr,                           // tp_clear
        nullptr,                           // tp_richcompare
        0,                                 // tp_weaklistoffset
        nullptr,                           // tp_iter
        nullptr,                           // tp_iternext
        nullptr,                           // tp_methods
        nullptr,                           // tp_members
        nullptr,                           // tp_getset
        nullptr,                           // tp_base
        nullptr,                           // tp_dict
        nullptr,                           // tp_descr_get
        nullptr,                           // tp_descr_set
        0,                                 // tp_dictoffset
        nullptr,                           // tp_init
        nullptr,                           // tp_alloc
        HomegearRpcMethod_new,             // tp_new
        nullptr,                           // tp_free
#if PY_MINOR_VERSION >= 8
        nullptr,                           // tp_is_gc
#endif
};
#endif

static PyObject *HomegearRpcMethod_new(PyTypeObject *type, PyObject *arg, PyObject *kw) {
  const char *methodName = nullptr;

  switch (PyTuple_Size(arg)) {
    case 1:if (!PyArg_ParseTuple(arg, "s", &methodName)) return nullptr;
      break;
    default:return nullptr;
  }

  if (!methodName) return nullptr;

  auto self = (HomegearRpcMethod *)type->tp_alloc(type, type->tp_basicsize);
  if (!self) return nullptr;
  //Py_INCREF(self); //valgrind does not complain if we don't do this and the dealloc is only called after setting the object to "None".
  self->methodName = new std::string(methodName);
  self->ipcClient = nullptr;
  self->nodeId = nullptr;

  return (PyObject *)self;
}

static void HomegearRpcMethod_dealloc(HomegearRpcMethod *self) {
  if (self->methodName) {
    delete self->methodName;
    self->methodName = nullptr;
  }

  Py_TYPE(self)->tp_free(self);
}

static PyObject *HomegearRpcMethod_call(PyObject *object, PyObject *args, PyObject *kw) {
  auto methodObject = (HomegearRpcMethod *)object;

  if (!methodObject->ipcClient) {
    Py_RETURN_NONE;
  }

  if (*methodObject->methodName == "connected") {
    if (methodObject->ipcClient->connected()) {
      Py_RETURN_TRUE;
    } else {
      Py_RETURN_FALSE;
    }
  }

  if (!methodObject->ipcClient->connected()) Py_RETURN_NONE;

  auto parameters = PythonVariableConverter::getVariable(args);

  auto nodeMethodIterator = kNodeMethods.find(*methodObject->methodName);
  if (nodeMethodIterator != kNodeMethods.end()) {
    if (!methodObject->nodeId || methodObject->nodeId->empty()) {
      PyErr_SetString(PyExc_Exception, "Node ID was not set in Object constructor.");
      return nullptr;
    }

    auto newParameters = std::make_shared<Ipc::Array>();
    newParameters->reserve(parameters->arrayValue->size() + 1);
    newParameters->emplace_back(std::make_shared<Ipc::Variable>(*methodObject->nodeId));
    newParameters->insert(newParameters->end(), parameters->arrayValue->begin(), parameters->arrayValue->end());

    auto result = methodObject->ipcClient->invoke(*methodObject->methodName, newParameters);
    if (result->errorStruct) {
      PyErr_SetString(PyExc_Exception, result->structValue->at("faultString")->stringValue.c_str());
      return nullptr;
    }

    return PythonVariableConverter::getPythonVariable(result);
  } else {
    auto result = methodObject->ipcClient->invoke(*methodObject->methodName, parameters->arrayValue);
    if (result->errorStruct) {
      PyErr_SetString(PyExc_Exception, result->structValue->at("faultString")->stringValue.c_str());
      return nullptr;
    }

    return PythonVariableConverter::getPythonVariable(result);
  }
}

static void Homegear_onConnect(HomegearObject *self) {
  std::unique_lock<std::mutex> waitLock(*self->onConnectWaitMutex);
  waitLock.unlock();
  self->onConnectConditionVariable->notify_all();
}

static void Homegear_broadcastEvent(HomegearObject *self, std::string &eventSource, uint64_t peerId, int32_t channel, std::string &variableName, Ipc::PVariable value) {
  if (!self->eventCallback) return;
  PyGILState_STATE gstate;
  gstate = PyGILState_Ensure();
  PyObject *pythonValue = PythonVariableConverter::getPythonVariable(value);
  if (pythonValue == nullptr) return;
  PyObject *arglist = Py_BuildValue("(sKisO)", eventSource.c_str(), (unsigned long long)peerId, (int)channel, variableName.c_str(), pythonValue);
  if (arglist == nullptr) {
    Py_DECREF(pythonValue);
    PyGILState_Release(gstate);
    return;
  }
  PyObject *result = PyObject_Call(self->eventCallback, arglist, nullptr);
  Py_DECREF(arglist);
  Py_DECREF(pythonValue);
  if (result == nullptr) {
    PyGILState_Release(gstate);
    return;
  }
  Py_DECREF(result);
  PyGILState_Release(gstate);
}

static void Homegear_nodeInput(HomegearObject *self, const Ipc::PVariable &nodeInfo, uint32_t inputIndex, const Ipc::PVariable &message) {
  if (!self->nodeInputCallback) return;
  PyGILState_STATE gstate;
  gstate = PyGILState_Ensure();
  PyObject *pythonNodeInfo = PythonVariableConverter::getPythonVariable(nodeInfo);
  PyObject *pythonMessage = PythonVariableConverter::getPythonVariable(message);
  if (nodeInfo == nullptr || pythonMessage == nullptr) return;
  PyObject *arglist = Py_BuildValue("(OIO)", pythonNodeInfo, (unsigned int)inputIndex, pythonMessage);
  if (arglist == nullptr) {
    Py_DECREF(pythonNodeInfo);
    Py_DECREF(pythonMessage);
    PyGILState_Release(gstate);
    return;
  }
  PyObject *result = PyObject_Call(self->nodeInputCallback, arglist, nullptr);
  Py_DECREF(arglist);
  Py_DECREF(pythonNodeInfo);
  Py_DECREF(pythonMessage);
  if (result == nullptr) {
    PyGILState_Release(gstate);
    return;
  }
  Py_DECREF(result);
  PyGILState_Release(gstate);
}

static PyObject *Homegear_new(PyTypeObject *type, PyObject *arg, PyObject *kw) {
  const char *socketPath = nullptr;
  PyObject *tempEventCallback = nullptr;
  const char *nodeId = nullptr;
  PyObject *tempNodeInputCallback = nullptr;

  switch (PyTuple_Size(arg)) {
    case 1: {
      if (!PyArg_ParseTuple(arg, "s", &socketPath)) return nullptr;
      break;
    }
    case 2: {
      if (!PyArg_ParseTuple(arg, "sO:Homegear_new", &socketPath, &tempEventCallback)) return nullptr;
      if (!PyCallable_Check(tempEventCallback)) {
        PyErr_SetString(PyExc_TypeError, "Parameter eventCallback must be callable.");
        return nullptr;
      }
      Py_XINCREF(tempEventCallback);         /* Add a reference to new callback */
      break;
    }
    case 4: {
      if (!PyArg_ParseTuple(arg, "sOsO:Homegear_new", &socketPath, &tempEventCallback, &nodeId, &tempNodeInputCallback)) return nullptr;
      if (!PyCallable_Check(tempEventCallback)) {
        PyErr_SetString(PyExc_TypeError, "Parameter eventCallback must be callable.");
        return nullptr;
      }
      Py_XINCREF(tempEventCallback);         /* Add a reference to new callback */

      if (!PyCallable_Check(tempNodeInputCallback)) {
        PyErr_SetString(PyExc_TypeError, "Parameter nodeInputCallback must be callable.");
        return nullptr;
      }
      Py_XINCREF(tempNodeInputCallback);         /* Add a reference to new callback */
      break;
    }
    default: {
      return nullptr;
    }
  }

  if (!socketPath) return nullptr;

  auto self = (HomegearObject *)type->tp_alloc(type, 0);
  if (!self) return nullptr;
  //Py_INCREF(self); //valgrind does not complain if we don't do this and the dealloc is only called after setting the object to "None".

  self->socketPath = new std::string(socketPath);
  if (self->socketPath->front() == '"' && self->socketPath->back() == '"') *self->socketPath = self->socketPath->substr(1, self->socketPath->length() - 2);

  self->eventCallback = tempEventCallback;
  self->nodeInputCallback = tempNodeInputCallback;
  if (nodeId) self->nodeId = new std::string(nodeId);
  else self->nodeId = new std::string();

  self->ipcClient = new IpcClient(*self->socketPath);

  self->onConnectConditionVariable = new std::condition_variable;
  self->onConnectWaitMutex = new std::mutex;

  return (PyObject *)self;
}

static int Homegear_init(HomegearObject *self, PyObject *arg) {
  if (self->eventCallback) {
    self->ipcClient->setBroadcastEvent(std::function<void(std::string &, uint64_t, int32_t, std::string &, Ipc::PVariable)>(std::bind(&Homegear_broadcastEvent,
                                                                                                                                      self,
                                                                                                                                      std::placeholders::_1,
                                                                                                                                      std::placeholders::_2,
                                                                                                                                      std::placeholders::_3,
                                                                                                                                      std::placeholders::_4,
                                                                                                                                      std::placeholders::_5)));
  }

  if (self->nodeInputCallback) {
    self->ipcClient->setNodeInput(std::function<void(const Ipc::PVariable &nodeInfo, uint32_t inputIndex, const Ipc::PVariable message)>(std::bind(&Homegear_nodeInput,
                                                                                                                                                   self,
                                                                                                                                                   std::placeholders::_1,
                                                                                                                                                   std::placeholders::_2,
                                                                                                                                                   std::placeholders::_3)));
  }

  self->ipcClient->setOnConnect(std::function<void(void)>(std::bind(&Homegear_onConnect, self)));
  self->ipcClient->start();
  std::unique_lock<std::mutex> waitLock(*self->onConnectWaitMutex);
  int64_t startTime = Ipc::HelperFunctions::getTime();
  while (!self->onConnectConditionVariable->wait_for(waitLock, std::chrono::milliseconds(2000), [&] {
    if (Ipc::HelperFunctions::getTime() - startTime > 2000) return true;
    else return self->ipcClient->connected();
  }));

  return 0;
}

static void Homegear_dealloc(HomegearObject *self) {
  if (self->eventCallback) {
    Py_XDECREF(self->eventCallback);
    self->eventCallback = nullptr;
  }

  if (self->nodeInputCallback) {
    Py_XDECREF(self->nodeInputCallback);
    self->nodeInputCallback = nullptr;
  }

  if (self->ipcClient) {
    delete self->ipcClient;
    self->ipcClient = nullptr;
  }

  if (self->nodeId) {
    delete self->nodeId;
    self->nodeId = nullptr;
  }

  if (self->socketPath) {
    delete self->socketPath;
    self->socketPath = nullptr;
  }

  if (self->onConnectConditionVariable) {
    delete self->onConnectConditionVariable;
    self->onConnectConditionVariable = nullptr;
  }

  if (self->onConnectWaitMutex) {
    delete self->onConnectWaitMutex;
    self->onConnectWaitMutex = nullptr;
  }

  Py_TYPE(self)->tp_free(self);
}

static PyObject *Homegear_call(PyObject *object, PyObject *attrName) {
  auto homegearObject = (HomegearObject *)object;

  if (!PyUnicode_Check(attrName)) {
    PyErr_Format(PyExc_TypeError, "Attribute name must be string, not '%.200s'", attrName->ob_type->tp_name);
    return nullptr;
  }

  Py_ssize_t methodNameSize = 0;
  const char *methodName = PyUnicode_AsUTF8AndSize(attrName, &methodNameSize);
  if (!methodName) return nullptr;

  auto homegearMethodObject = (HomegearRpcMethod *)HomegearRpcMethodType.tp_alloc(&HomegearRpcMethodType, 0);
  if (!homegearMethodObject) return nullptr;
  //Py_INCREF(homegearMethodObject); //valgrind does not complain if we don't do this and the dealloc is only called after setting the object to "None".

  homegearMethodObject->methodName = new std::string(methodName, methodNameSize);
  homegearMethodObject->ipcClient = homegearObject->ipcClient;
  homegearMethodObject->nodeId = homegearObject->nodeId;

  return (PyObject *)homegearMethodObject;
}

static struct PyModuleDef HomegearModule = {
    PyModuleDef_HEAD_INIT,
    "homegear",   /* name of module */
    nullptr, /* module documentation, may be NULL */
    -1,       /* size of per-interpreter state of the module, or -1 if the module keeps state in global variables. */
    nullptr
};

PyMODINIT_FUNC PyInit_homegear(void) {
  PyEval_InitThreads(); //Bugfix - Not needed in Python 3.6+, no-op when called for a second time. Must be called from the main thread.

  if (PyType_Ready(&HomegearObjectType) < 0 || PyType_Ready(&HomegearRpcMethodType) < 0) return nullptr;

  PyObject *m = PyModule_Create(&HomegearModule);

  if (!m) return nullptr;

  Py_INCREF(&HomegearObjectType);
  PyModule_AddObject(m, "Homegear", (PyObject *)&HomegearObjectType);

  Py_INCREF(&HomegearRpcMethodType);
  PyModule_AddObject(m, "HomegearRpcMethod", (PyObject *)&HomegearRpcMethodType);

  return m;
}
