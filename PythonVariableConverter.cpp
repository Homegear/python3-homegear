/* Copyright 2013-2019 Homegear GmbH
*
* Homegear is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* Homegear is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with Homegear.  If not, see <http://www.gnu.org/licenses/>.
*
* In addition, as a special exception, the copyright holders give
* permission to link the code of portions of this program with the
* OpenSSL library under certain conditions as described in each
* individual source file, and distribute linked combinations
* including the two.
* You must obey the GNU General Public License in all respects
* for all of the code used other than OpenSSL.  If you modify
* file(s) with this exception, you may extend this exception to your
* version of the file(s), but you are not obligated to do so.  If you
* do not wish to do so, delete this exception statement from your
* version.  If you delete this exception statement from all source
* files in the program, then also delete it here.
*/

#include "PythonVariableConverter.h"

Ipc::PVariable PythonVariableConverter::getVariable(PyObject *value) {
  Ipc::PVariable variable;
  if (!value) return std::make_shared<Ipc::Variable>();

  if (PyTuple_Check(value)) {
    variable = std::make_shared<Ipc::Variable>(Ipc::VariableType::tArray);
    Py_ssize_t tupleSize = PyTuple_Size(value);
    variable->arrayValue->reserve(tupleSize);
    for (int32_t i = 0; i < tupleSize; i++) {
      auto arrayElement = getVariable(PyTuple_GetItem(value, i));
      if (arrayElement) variable->arrayValue->emplace_back(arrayElement);
    }
  } else if (PyList_Check(value)) {
    variable = std::make_shared<Ipc::Variable>(Ipc::VariableType::tArray);
    Py_ssize_t listSize = PyList_Size(value);
    variable->arrayValue->reserve(listSize);
    for (int32_t i = 0; i < listSize; i++) {
      auto arrayElement = getVariable(PyList_GetItem(value, i));
      if (arrayElement) variable->arrayValue->emplace_back(arrayElement);
    }
  } else if (PyDict_Check(value)) {
    variable = std::make_shared<Ipc::Variable>(Ipc::VariableType::tStruct);
    PyObject *key = nullptr;
    PyObject *dictElement = nullptr;
    Py_ssize_t pos = 0;
    while (PyDict_Next(value, &pos, &key, &dictElement)) {
      if (!key || !dictElement) continue;
      auto structKey = getVariable(key);
      auto structElement = getVariable(dictElement);
      if (structKey && structElement) variable->structValue->emplace(structKey->toString(), structElement);
    }
  } else if (PyBool_Check(value)) variable = std::make_shared<Ipc::Variable>((bool)PyObject_IsTrue(value));
  else if (PyLong_Check(value)) variable = std::make_shared<Ipc::Variable>((int64_t)PyLong_AsLongLong(value));
  else if (PyFloat_Check(value)) variable = std::make_shared<Ipc::Variable>(PyFloat_AsDouble(value));
  else if (PyUnicode_Check(value)) {
    Py_ssize_t stringSize = 0;
    const char *utf8String = PyUnicode_AsUTF8AndSize(value, &stringSize); //From the documentation: "The caller is not responsible for deallocating the buffer."
    if (utf8String) variable = std::make_shared<Ipc::Variable>(std::string(utf8String, stringSize));
    else variable = std::make_shared<Ipc::Variable>(Ipc::VariableType::tString);
  } else if (PyBytes_Check(value)) {
    char *rawByteArray = PyBytes_AsString(value);
    std::vector<char> byteArray;
    if (rawByteArray) byteArray = std::vector<char>(rawByteArray, rawByteArray + PyBytes_Size(value));
    variable = std::make_shared<Ipc::Variable>(byteArray);
  } else if (value == Py_None) {
    variable = std::make_shared<Ipc::Variable>(Ipc::VariableType::tVoid);
  } else {
    variable = std::make_shared<Ipc::Variable>();
  }

  return variable;
}

PyObject *PythonVariableConverter::getPythonVariable(const Ipc::PVariable &input) {
  PyObject *output = nullptr;
  if (!input) return output;

  if (input->type == Ipc::VariableType::tArray) {
    output = PyList_New(input->arrayValue->size());
    for (int32_t i = 0; i < (int32_t)input->arrayValue->size(); i++) {
      auto value = getPythonVariable(input->arrayValue->at(i));
      if (value == nullptr) continue;
      PyList_SetItem(output, i, value);
    }
  } else if (input->type == Ipc::VariableType::tStruct) {
    output = PyDict_New();
    for (auto &element : *input->structValue) {
      auto key = Py_BuildValue("s", element.first.c_str());
      if (key == nullptr) continue;
      auto value = getPythonVariable(element.second);
      if (value == nullptr) continue;
      PyDict_SetItem(output, key, value);
    }
  } else if (input->type == Ipc::VariableType::tVoid) {
    Py_INCREF(Py_None);
    output = Py_None;
  } else if (input->type == Ipc::VariableType::tBoolean) {
    if (input->booleanValue) {
      Py_INCREF(Py_True);
      output = Py_True;
    } else {
      Py_INCREF(Py_False);
      output = Py_False;
    }
  } else if (input->type == Ipc::VariableType::tInteger) {
    output = Py_BuildValue("l", (long)input->integerValue);
  } else if (input->type == Ipc::VariableType::tInteger64) {
    output = Py_BuildValue("L", (long long)input->integerValue64);
  } else if (input->type == Ipc::VariableType::tFloat) {
    output = Py_BuildValue("d", input->floatValue);
  } else if (input->type == Ipc::VariableType::tString || input->type == Ipc::VariableType::tBase64) {
    output = Py_BuildValue("s", input->stringValue.c_str());
  } else if (input->type == Ipc::VariableType::tBinary) {
    output = Py_BuildValue("y*", (char *)input->binaryValue.data());
  } else {
    output = Py_BuildValue("s", "UNKNOWN");
  }
  return output;
}

