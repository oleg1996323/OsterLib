add_test([=[JsonParse.JsonArrayParseTest]=]  /home/oster/OsterLib/tests/parse_json_test [==[--gtest_filter=JsonParse.JsonArrayParseTest]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[JsonParse.JsonArrayParseTest]=]  PROPERTIES DEF_SOURCE_LINE /home/oster/OsterLib/tests/parse_json.cpp:4 WORKING_DIRECTORY /home/oster/OsterLib/tests SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==])
set(  parse_json_test_TESTS JsonParse.JsonArrayParseTest)
