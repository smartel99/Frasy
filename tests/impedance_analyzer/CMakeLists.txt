add_executable(FrasyTest_ImpedanceAnalyzer
    test.cpp
)
target_link_libraries(FrasyTest_ImpedanceAnalyzer PRIVATE Frasy GTest::gtest_main)
target_include_directories(FrasyTest_ImpedanceAnalyzer PRIVATE ${CMAKE_CURRENT_LIST_DIR}/..)
gtest_discover_tests(FrasyTest_ImpedanceAnalyzer)
