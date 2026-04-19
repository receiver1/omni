include(CTest)
include(OmniUseBoostUT)

message(STATUS "[OMNI] Configuring tests")

file(GLOB_RECURSE omni_unit_test_sources CONFIGURE_DEPENDS
  "${CMAKE_CURRENT_LIST_DIR}/*.cpp"
)

list(SORT omni_unit_test_sources)

set(omni_unit_test_main "${CMAKE_CURRENT_LIST_DIR}/main.cpp")
list(REMOVE_ITEM omni_unit_test_sources "${omni_unit_test_main}")

# boost.ut registers global suites during dynamic initialization.
# Keep the runner TU first so suite registration order stays stable on Windows.
list(PREPEND omni_unit_test_sources "${omni_unit_test_main}")

add_executable(omni_tests)
omni_use_boost_ut(omni_tests)

target_sources(omni_tests PRIVATE ${omni_unit_test_sources})
target_compile_features(omni_tests PRIVATE cxx_std_23)
target_include_directories(omni_tests PRIVATE ${omni_INCLUDE_DIR} ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(omni_tests PRIVATE omni::omni)

add_test(NAME omni_tests COMMAND omni_tests)
