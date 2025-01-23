#!/bin/bash
sed -i -E "/tag=/! s/name='(.*)',type/name='\1',tag='in',type/g" generator_cl.py #Add tag where it doesn't exist default is in
sed -i -E "/type='.*\*'/ s/tag='in'/tag='out'/g" generator_cl.py #Change from tag in to out if type is a pointer
sed -i -E "/type='const .*'/ s/tag='out'/tag='in'/g" generator_cl.py #Change from tag out to in if pointer is const