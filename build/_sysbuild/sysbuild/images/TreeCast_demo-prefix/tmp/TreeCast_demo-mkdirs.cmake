# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file LICENSE.rst or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION ${CMAKE_VERSION}) # this file comes with cmake

# If CMAKE_DISABLE_SOURCE_CHANGES is set to true and the source directory is an
# existing directory in our source tree, calling file(MAKE_DIRECTORY) on it
# would cause a fatal error, even though it would be a no-op.
if(NOT EXISTS "/Users/sayon/Documents/codes/TreeCast_demo")
  file(MAKE_DIRECTORY "/Users/sayon/Documents/codes/TreeCast_demo")
endif()
file(MAKE_DIRECTORY
  "/Users/sayon/Documents/codes/TreeCast_demo/build/TreeCast_demo"
  "/Users/sayon/Documents/codes/TreeCast_demo/build/_sysbuild/sysbuild/images/TreeCast_demo-prefix"
  "/Users/sayon/Documents/codes/TreeCast_demo/build/_sysbuild/sysbuild/images/TreeCast_demo-prefix/tmp"
  "/Users/sayon/Documents/codes/TreeCast_demo/build/_sysbuild/sysbuild/images/TreeCast_demo-prefix/src/TreeCast_demo-stamp"
  "/Users/sayon/Documents/codes/TreeCast_demo/build/_sysbuild/sysbuild/images/TreeCast_demo-prefix/src"
  "/Users/sayon/Documents/codes/TreeCast_demo/build/_sysbuild/sysbuild/images/TreeCast_demo-prefix/src/TreeCast_demo-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/Users/sayon/Documents/codes/TreeCast_demo/build/_sysbuild/sysbuild/images/TreeCast_demo-prefix/src/TreeCast_demo-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/Users/sayon/Documents/codes/TreeCast_demo/build/_sysbuild/sysbuild/images/TreeCast_demo-prefix/src/TreeCast_demo-stamp${cfgdir}") # cfgdir has leading slash
endif()
