# Test executables will be stored in a subfolder called "tests" in the
# deployment folder.

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_DEPLOY_DIR}/tests)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_DEPLOY_DIR}/tests)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_DEPLOY_DIR}/tests)

add_subdirectory(AddmusicK)