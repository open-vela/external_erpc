#! /usr/bin/python

# Copyright 2016-2021 NXP
# All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause

# To use this script run
# $./run_clang_format.py
from __future__ import print_function
import subprocess
import sys
import os
import re

#Folders to scan
folders = []
folders.append("erpc_c");
folders.append("erpcgen/src");
folders.append("erpcsniffer/src");
folders.append("test");

#Files which will be not formatted
exceptions = []
exceptions.append("test/common/gtest/gtest.h");
exceptions.append("test/common/gtest/gtest.cpp");
exceptions.append("erpcgen/src/cpptemplate/cpptempl.h");
exceptions.append("erpcgen/src/cpptemplate/cpptempl.cpp");
exceptions.append("erpcgen/src/cpptemplate/cpptempl_test.cpp");

#For windows use "\\" instead of "/" path separators.
if os.environ.get('OS','') == 'Windows_NT':
    for i, folder in enumerate(folders):
        folders[i] = os.path.normpath(folder)

    for i, ext in enumerate(exceptions):
        exceptions[i] = os.path.normpath(ext)

#Files with this extensions will be formatted/
extensions = []
extensions.append(".h")
extensions.append(".hpp")
extensions.append(".c")
extensions.append(".cpp")

# Check that the clang-format is installed and >= the minimum version
clang_format_minimum_version = [10, 0, 0]
try:
    cf = subprocess.Popen(
        ["clang-format", "--version"], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
except FileNotFoundError:
    print("clang-format is not in $PATH")
    exit(1)
clang_format_stdout, clang_format_stderr = cf.communicate()
if cf.returncode != 0:
    print("Failed to determine clang-format version. stderr=\"%s\"" % (
        clang_format_stderr.decode('utf-8')))
    exit(1)
clang_format_stdout = clang_format_stdout.decode("utf-8")
m = re.match('^clang-format version (\d+\.\d+\.\d+)', clang_format_stdout)
if m is None:
    print("clang-format --version output not understood: \"%s\"" % (clang_format_stdout))
    exit(1)
clang_format_version = [int(e) for e in m.group(1).split(".")]

if (clang_format_version[0] < clang_format_minimum_version[0]) or \
    ((clang_format_version[0] == clang_format_minimum_version[0]) and \
     ((clang_format_version[1] < clang_format_minimum_version[1]) or \
      ((clang_format_version[1] == clang_format_minimum_version[1]) and \
       (clang_format_version[2] < clang_format_minimum_version[2])))):
     print(
         "Installed clang-format version (%d.%d.%d) is less than the required minimum version (%d.%d.%d)" % (
             clang_format_version[0],
             clang_format_version[1],
             clang_format_version[2],
             clang_format_minimum_version[0],
             clang_format_minimum_version[1],
             clang_format_minimum_version[2]))
     exit(1)

#processing formatting
for folder in folders:
    print('*****************************************************************************')
    print(folder);
    for path, subdirs, files in os.walk(folder):
        for name in files:
            if any(ext in name for ext in extensions):
                file = os.path.join(path, name)
                if file in exceptions:
                    print("Ignored: ", file)
                else:
                    print("Formatting: ", file)
                    subprocess.call(["clang-format-10.0", "-i", file])
    print('*****************************************************************************\n')
