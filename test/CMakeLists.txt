file(GLOB sources *.cpp *.h ../include/flat_container/*.h ../include/flat_container/*.natvis)
add_executable(Test ${sources})
target_include_directories(Test PUBLIC "${CMAKE_SOURCE_DIR}/include")

