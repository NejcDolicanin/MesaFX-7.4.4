#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""
Generate dispatch table files with proper UTF-8 encoding for Windows builds.
This script runs the Mesa GL API generation scripts and saves output as UTF-8.
"""

import sys
import os
import codecs

# Change to the glapi directory
script_dir = os.path.dirname(os.path.abspath(__file__))
os.chdir(script_dir)

# List of scripts to run and their output files
generators = [
    ('gl_procs.py', 'glprocs.h'),
    ('gl_apitemp.py', 'glapitemp.h'),
    ('gl_offsets.py', 'glapioffsets.h'),
    ('gl_table.py', 'glapitable.h'),
]

print("Generating dispatch table files with UTF-8 encoding...")

for script, output in generators:
    print("Running %s -> %s" % (script, output))
    
    # Capture stdout
    old_stdout = sys.stdout
    sys.stdout = codecs.open(output, 'w', encoding='utf-8')
    
    try:
        # Execute the script
        execfile(script, {'__name__': '__main__'})
    finally:
        sys.stdout.close()
        sys.stdout = old_stdout
    
    print("  Generated %s" % output)

# Generate dispatch.h with remap_table mode
print("Running gl_table.py -m remap_table -> dispatch.h")
old_stdout = sys.stdout
old_argv = sys.argv
sys.stdout = codecs.open('dispatch.h', 'w', encoding='utf-8')
sys.argv = ['gl_table.py', '-m', 'remap_table']

try:
    execfile('gl_table.py', {'__name__': '__main__'})
finally:
    sys.stdout.close()
    sys.stdout = old_stdout
    sys.argv = old_argv

print("  Generated dispatch.h")

print("\nAll files generated successfully with UTF-8 encoding!")
