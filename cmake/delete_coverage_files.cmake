file(GLOB_RECURSE PLSSVM_COVERAGE_GCDA_FILES ${CMAKE_BINARY_DIR}/*.gcda)
file(REMOVE ${PLSSVM_COVERAGE_GCDA_FILES})
file(GLOB_RECURSE PLSSVM_COVERAGE_GCNO_FILES ${CMAKE_BINARY_DIR}/*.gcno)
file(REMOVE ${PLSSVM_COVERAGE_GCNO_FILES})