from distutils.core import setup, Extension
setup(
	name="homegear",
	version="1.0",
	description = 'Extension to connect to a local Homegear service.',
	author="Homegear GmbH",
	author_email="contact@homegear.email",
	url="https://github.com/Homegear/libhomegear-python",
	download_url = 'https://github.com/Homegear/libhomegear-python/archive/1.0.tar.gz',
	keywords = ['homegear', 'smart home'],
	ext_modules=[
		Extension("homegear", ["homegear.cpp", "IpcClient.cpp", "PythonVariableConverter.cpp"],
		extra_compile_args=['-std=c++11'],
		extra_link_args=['-lhomegear-ipc'])
	]
)
