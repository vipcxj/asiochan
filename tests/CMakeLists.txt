add_executable(asiochan_tests)
add_test(
  NAME asiochan_tests
  COMMAND asiochan_tests
)
target_include_directories(
  asiochan_tests

  PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}"
  "${CMAKE_CURRENT_BINARY_DIR}"
)
target_compile_definitions(
  asiochan_tests

  PRIVATE
  ASIOCHAN_CH_ALLOCATE_TRACER
)
find_package(Catch2 CONFIG REQUIRED)
target_link_libraries(
  asiochan_tests

  PRIVATE
  Catch2::Catch2 Catch2::Catch2WithMain
  Threads::Threads
  asiochan::asiochan
)

add_subdirectory(asiochan)
