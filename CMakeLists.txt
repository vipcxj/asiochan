cmake_minimum_required(VERSION 3.17)
set(ASIOCHAN_VERSION 0.4.4)
set(LIB_NAME asiochan)
project(AsioChan VERSION "${ASIOCHAN_VERSION}")

include(CheckCXXCompilerFlag)

option(ASIOCHAN_USE_STANDALONE_ASIO "Use standalone ASIO instead of Boost.ASIO" OFF)
option(ENABLE_TESTING "Enable unit testing." ON)

add_library(${LIB_NAME} INTERFACE)
add_library(${LIB_NAME}::${LIB_NAME} ALIAS ${LIB_NAME})
target_compile_features(${LIB_NAME} INTERFACE cxx_std_20)
target_include_directories(${LIB_NAME} INTERFACE 
  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)

set(COROUTINES_FLAG -fcoroutines)
check_cxx_compiler_flag("${COROUTINES_FLAG}" COMPILER_HAS_COROUTINES_FLAG)
if (COMPILER_HAS_COROUTINES_FLAG)
  target_compile_options(${LIB_NAME} INTERFACE "${COROUTINES_FLAG}")
endif()

if (ASIOCHAN_USE_STANDALONE_ASIO)
  target_compile_definitions(${LIB_NAME} INTERFACE ASIOCHAN_USE_STANDALONE_ASIO)
endif()

include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    "${PROJECT_BINARY_DIR}/asiochanConfigVersion.cmake"
    VERSION @VERSION@
    COMPATIBILITY AnyNewerVersion
)

install(TARGETS ${LIB_NAME}
    EXPORT ${LIB_NAME}Targets
    PUBLIC_HEADER DESTINATION include
)

configure_package_config_file(
    "${PROJECT_SOURCE_DIR}/${LIB_NAME}Config.cmake"
    "${PROJECT_BINARY_DIR}/${LIB_NAME}Config.cmake"
    INSTALL_DESTINATION share/${LIB_NAME}
)

install(EXPORT ${LIB_NAME}Targets DESTINATION share/${LIB_NAME})
install(FILES "${PROJECT_BINARY_DIR}/${LIB_NAME}ConfigVersion.cmake"
    "${PROJECT_BINARY_DIR}/${LIB_NAME}Config.cmake"
    DESTINATION share/${LIB_NAME})
install(DIRECTORY ${PROJECT_SOURCE_DIR}/include/asiochan DESTINATION include)

if(ENABLE_TESTING)
  find_package(Boost REQUIRED)
  if (ASIOCHAN_USE_STANDALONE_ASIO)
    target_link_libraries(asiochan INTERFACE asio::asio)
  else()
    target_link_libraries(asiochan INTERFACE Boost::boost)
  endif()

  find_package(Threads REQUIRED)
  enable_testing()

  add_subdirectory(examples)
  add_subdirectory(tests)
endif()

