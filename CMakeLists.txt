cmake_minimum_required(VERSION 3.10.0)
project(growtopia-http-v2 VERSION 0.1.0 LANGUAGES C)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS}")

# Output directory configuration
# Binaries go to dist/bin/, intermediate build files to build/
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/dist/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/build")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/build")

# Install prefix
if (NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "${CMAKE_SOURCE_DIR}" CACHE PATH "Installation prefix" FORCE)
endif()

include_directories(
  include
  externals/h2o/include
  externals/h2o/deps/picotls/include
  externals/h2o/deps/quicly/include
)

find_package(OpenSSL REQUIRED)
find_package(Threads REQUIRED)

add_subdirectory(externals/h2o EXCLUDE_FROM_ALL)

add_executable(server
  src/main.c
)

# Build a small internal library for app handlers and helpers
add_library(growtopia STATIC src/handlers.c src/listener.c src/security.c)
target_include_directories(growtopia PUBLIC ${PROJECT_SOURCE_DIR}/include)
# Ensure POSIX feature macros and pthread linkage are visible when building the library
target_compile_definitions(growtopia PRIVATE _POSIX_C_SOURCE=200809L)
target_link_libraries(growtopia PRIVATE Threads::Threads)

# Ensure POSIX feature macros are available when compiling (posix_memalign, addrinfo, etc.)
target_compile_definitions(server PRIVATE _POSIX_C_SOURCE=200809L)
# Link to pthreads so headers like <pthread.h> provide the correct types during compilation
target_link_libraries(server PRIVATE Threads::Threads)

# expose server config as a PCH for fast iteration (optional)
target_precompile_headers(server PRIVATE
  include/growtopia/config/server.h
)

# link the app library and external deps
target_link_libraries(server PRIVATE growtopia libh2o OpenSSL::SSL OpenSSL::Crypto)

# Install rules: put runtime and libraries under the configured prefix (defaults to ${DEFAULT_OUT_DIR})
install(TARGETS server growtopia
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
)

# Install public web root (optional)
install(DIRECTORY ${CMAKE_SOURCE_DIR}/public/ DESTINATION share/growtopia/public)
