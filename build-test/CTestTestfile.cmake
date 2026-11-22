# CMake generated Testfile for 
# Source directory: /home/buzz071/projects/trrispo25
# Build directory: /home/buzz071/projects/trrispo25/build-test
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test([=[core]=] "/home/buzz071/projects/trrispo25/build-test/test_core")
set_tests_properties([=[core]=] PROPERTIES  _BACKTRACE_TRIPLES "/home/buzz071/projects/trrispo25/CMakeLists.txt;40;add_test;/home/buzz071/projects/trrispo25/CMakeLists.txt;0;")
add_test([=[cli_version]=] "/home/buzz071/projects/trrispo25/build-test/trrispo25" "--version")
set_tests_properties([=[cli_version]=] PROPERTIES  PASS_REGULAR_EXPRESSION "0.2.0" _BACKTRACE_TRIPLES "/home/buzz071/projects/trrispo25/CMakeLists.txt;43;add_test;/home/buzz071/projects/trrispo25/CMakeLists.txt;0;")
add_test([=[cli_bad_args]=] "/home/buzz071/projects/trrispo25/build-test/trrispo25" "--no-such-option")
set_tests_properties([=[cli_bad_args]=] PROPERTIES  WILL_FAIL "TRUE" _BACKTRACE_TRIPLES "/home/buzz071/projects/trrispo25/CMakeLists.txt;48;add_test;/home/buzz071/projects/trrispo25/CMakeLists.txt;0;")
