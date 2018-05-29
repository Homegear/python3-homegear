from distutils.core import setup, Extension
setup(name="homegear", version="1.0", ext_modules=[Extension("homegear", ["homegear.cpp", "IpcClient.cpp", "PythonVariableConverter.cpp"], extra_compile_args=['-std=c++11'], extra_link_args=['-lhomegear-ipc'])])
