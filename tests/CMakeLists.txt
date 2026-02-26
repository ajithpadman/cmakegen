# GTest from FetchContent in parent

# Generate full_metadata.json for comprehensive E2E test
# freertos_kernel uses git (cloned by cmakegen); hal, uart_driver, main_app use local dummy_sources
set(DUMMY_SOURCES_DIR ${CMAKE_SOURCE_DIR}/tests/e2e/fixtures/dummy_sources)
configure_file(
    ${CMAKE_SOURCE_DIR}/tests/e2e/fixtures/full_metadata.json.in
    ${CMAKE_CURRENT_BINARY_DIR}/full_metadata.json
    @ONLY
)
configure_file(
    ${CMAKE_SOURCE_DIR}/tests/e2e/fixtures/arm_compile_metadata.json.in
    ${CMAKE_CURRENT_BINARY_DIR}/arm_compile_metadata.json
    @ONLY
)

add_executable(parser_test unit/parser_test.cpp)
target_link_libraries(parser_test PRIVATE cmakegen_lib GTest::gtest GTest::gtest_main)
target_include_directories(parser_test PRIVATE ${CMAKE_SOURCE_DIR}/src)
add_test(NAME ParserTest COMMAND parser_test)

add_executable(filter_test unit/filter_test.cpp)
target_link_libraries(filter_test PRIVATE cmakegen_lib GTest::gtest GTest::gtest_main)
target_include_directories(filter_test PRIVATE ${CMAKE_SOURCE_DIR}/src)
add_test(NAME FilterTest COMMAND filter_test)

add_executable(condition_evaluator_test unit/condition_evaluator_test.cpp)
target_link_libraries(condition_evaluator_test PRIVATE cmakegen_lib GTest::gtest GTest::gtest_main)
target_include_directories(condition_evaluator_test PRIVATE ${CMAKE_SOURCE_DIR}/src)
add_test(NAME ConditionEvaluatorTest COMMAND condition_evaluator_test)

add_executable(integration_test integration/pipeline_test.cpp)
target_link_libraries(integration_test PRIVATE cmakegen_lib GTest::gtest GTest::gtest_main)
target_include_directories(integration_test PRIVATE ${CMAKE_SOURCE_DIR}/src)
add_test(NAME IntegrationTest COMMAND integration_test)

add_executable(e2e_test e2e/e2e_test.cpp)
target_link_libraries(e2e_test PRIVATE cmakegen_lib GTest::gtest GTest::gtest_main)
target_include_directories(e2e_test PRIVATE ${CMAKE_SOURCE_DIR}/src)
add_test(NAME E2ETest COMMAND e2e_test WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
set_tests_properties(E2ETest PROPERTIES ENVIRONMENT
  "E2E_FULL_METADATA=${CMAKE_CURRENT_BINARY_DIR}/full_metadata.json;E2E_ARM_COMPILE_METADATA=${CMAKE_CURRENT_BINARY_DIR}/arm_compile_metadata.json")
