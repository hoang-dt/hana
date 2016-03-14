add_executable(tests tests.cpp md5.cpp)
target_link_libraries(tests hana_idx hana_core)
set_target_properties(tests PROPERTIES CXX_STANDARD 14)

if(MSVC)
    target_compile_definitions(tests PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()