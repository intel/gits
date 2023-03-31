# GITS-OCL-generator
Generator for GITS OpenCL

## Prerequisites

### generatorCreatorCL.py

1. `utils/fake_libc_include` - from pycparser [1]
2. CL headers to generate definitions from in `CL` directory

### gitsCLFuncsGenerator.py

1. `openclIDs.h` - to add the new IDs

# Running
1. `python generatorCreatorCL.py <CL-version>` - will add definitions from
headers to generator_cl.py, CL-version will be put in functions' version field

2. `python gitsCLFuncsGenerator.py` - will generate the headers

[1] https://github.com/eliben/pycparser
