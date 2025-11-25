#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""
Generate x86 assembly dispatch file with proper UTF-8 encoding for Windows builds.
"""

import sys
import os
import codecs

# Change to the glapi directory
script_dir = os.path.dirname(os.path.abspath(__file__))
os.chdir(script_dir)

print("Generating x86 assembly dispatch file with UTF-8 encoding...")

# Generate glapi_x86.S
output_file = '../x86/glapi_x86.S'
print("Running gl_x86_asm.py -> %s" % output_file)

old_stdout = sys.stdout
sys.stdout = codecs.open(output_file, 'w', encoding='utf-8')

try:
    execfile('gl_x86_asm.py', {'__name__': '__main__'})
finally:
    sys.stdout.close()
    sys.stdout = old_stdout

print("  Generated %s" % output_file)
print("\nX86 assembly file generated successfully with UTF-8 encoding!")
