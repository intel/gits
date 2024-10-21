# GITS-OCL-generator
Generator for GITS OpenCL

## Prerequisites
### generate.py
1. generator_cl.py - to add new API calls, enums and structures

### generatorCreatorCL.py

1. `utils/fake_libc_include` - from pycparser [1]
2. CL headers to generate definitions from in `CL` directory

# Running
1. `python generatorCreatorCL.py <CL-version>` - will add definitions from
headers to generator_cl.py, CL-version will be put in functions' version field

2. `python generate.py <Out-Dir>` - will generate files using mako templates 

3. `python generate.py update` - will add new API calls to existing files using mako templates

[1] https://github.com/eliben/pycparser
