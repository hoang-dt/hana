add_executable(tests tests.cpp md5.cpp)
target_link_libraries(tests core idx)
set_target_properties(tests PROPERTIES CXX_STANDARD 14)