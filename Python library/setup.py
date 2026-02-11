from setuptools import setup, Extension
import os

# Define the C extension module
xton_module = Extension(
    'xton._c_ext',  # Name of the C extension module
    sources=['src/xton.c'],  # Source files for the C extension
    include_dirs=[os.path.join(os.sys.prefix, 'include')], # Include Python headers
    library_dirs=[os.path.join(os.sys.prefix, 'lib')],     # Link Python libraries
    # You might need to add -lpython<version> depending on your system
    # extra_link_args=['-lpython3.12'] 
)

setup(
    name='pyxton',
    version='0.1.0',
    author='Your Name', # Replace with actual author
    author_email='your.email@example.com', # Replace with actual email
    description='A C extension based Python library for XTon parsing and serialization',
    long_description=open('README.md').read(),
    long_description_content_type='text/markdown',
    ext_modules=[xton_module],
    packages=['xton'], # This tells setuptools to look for a package named 'xton'
    classifiers=[
        'Programming Language :: Python :: 3',
        'Programming Language :: Python :: 3.8',
        'Programming Language :: Python :: 3.9',
        'Programming Language :: Python :: 3.10',
        'Programming Language :: Python :: 3.11',
        'Programming Language :: Python :: 3.12',
        'License :: OSI Approved :: MIT License',
        'Operating System :: OS Independent',
    ],
    python_requires='>=3.8',
)