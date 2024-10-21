metagenerator works on L0 YAML files.

1. Clone Level Zero spec from https://github.com/oneapi-src/level-zero-spec
2. Get contents of `scripts` directory, copy them into ./vendor folder
3. Add ./vendor path to `PYTHONPATH` environment variable
4. Obtain `*_ddi.h` header files from https://github.com/oneapi-src/level-zero
5. Run `python3 metagen.py .`
# Import callbacks from level zero loader
6. Run `python3 manual.py ./level-zero/include/layers/zel_tracing_register_cb.h ../generator_l0.py`

import tweaks may be required.
