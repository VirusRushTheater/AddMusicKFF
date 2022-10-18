#!/usr/bin/env python
# Designed as an OS-agnostic build script, because of how different PowerShell
# and bash treat environment variables.

import os
import argparse
from subprocess import check_call

argp = argparse.ArgumentParser(description="OS-agnostic building script for usage with Appveyor")
argp.add_argument("--step", help="Building step")
args = argp.parse_args()

assert(args.step in ["configure", "build", "deploy"])

CONFIGURATION = os.environ['CONFIGURATION']
PLATFORM = 		os.environ['PLATFORM']
PATH = 			os.environ['PATH']

cmake_command = ['cmake', '-DFMT_PEDANTIC=ON', '-DCMAKE_BUILD_TYPE=' + config]

os.environ['PATH'] = r'C:\Program Files (x86)\MSBuild\14.0\Bin;' + path
build_command = ['msbuild', '/m:4', '/p:Config=' + config, 'foo.sln']
#test_command = ['msbuild', 'RUN_TESTS.vcxproj']

check_call(cmake_command)
check_call(build_command)
#check_call(test_command)