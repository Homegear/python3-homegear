import setuptools
from distutils.core import Extension

with open("README.md", "r") as fh:
    long_description = fh.read()

setuptools.setup(
	name="homegear",
	version="1.0.5",
	description = 'Extension to connect to a local Homegear service.',
	long_description=long_description,
	long_description_content_type="text/markdown",
	author="Homegear GmbH",
	author_email="contact@homegear.email",
	url="https://github.com/Homegear/libhomegear-python",
	keywords = ['homegear', 'smart home'],
	ext_modules=[
		Extension("homegear", ["homegear.cpp", "IpcClient.cpp", "PythonVariableConverter.cpp"],
		extra_compile_args=['-std=c++11'],
		extra_link_args=['-lhomegear-ipc'])
	],
	include_package_data=True,
	classifiers=(
		"Programming Language :: C++",
		"License :: OSI Approved :: GNU Lesser General Public License v3 (LGPLv3)",
		"Operating System :: POSIX"
	)
)
