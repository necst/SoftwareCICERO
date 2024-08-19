
add_executable(
        test_multi
        newTestMulti.cpp
)

target_link_libraries(
        test_multi
        CiceroMulti
)

include(CTest)

add_test(
        NAME test_multi
        COMMAND test_multi
)

target_compile_definitions(
        test_multi
        PRIVATE
        TEST_INPUT_PATH="${CMAKE_CURRENT_SOURCE_DIR}/"
)