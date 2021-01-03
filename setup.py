import setuptools
import sys
from distutils.core import Extension

if sys.version_info < (3,0):
    sys.exit('Sorry, Python < 3.0 is not supported')

with open("README.md", "r") as fh:
    long_description = fh.read()

version = None
with open('version.txt') as f: version = f.read().strip()

setuptools.setup(
	name="homegear",
	version=version,
	description = 'Extension to connect to a local Homegear service.',
	long_description=long_description,
	long_description_content_type="text/markdown",
	author="Homegear GmbH",
	author_email="contact@homegear.email",
	url="https://github.com/Homegear/python3-homegear",
	keywords = ['homegear', 'smart home'],
	ext_modules=[
		Extension("homegear", ["homegear.cpp", "IpcClient.cpp", "PythonVariableConverter.cpp"],
		extra_compile_args=['-std=c++17'],
		extra_link_args=['-lhomegear-ipc', '-latomic'])
	],
	include_package_data=True,
	classifiers=(
		"Programming Language :: C++",
		"License :: OSI Approved :: GNU Lesser General Public License v3 (LGPLv3)",
		"Operating System :: POSIX"
	)
)
