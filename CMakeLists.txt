cmake_minimum_required(VERSION 3.31)
project(lppm CXX)

set(LPPM_SRC
    src/cli.cpp
    src/globals.cpp
    src/handlers.cpp
    src/main.cpp
    src/os.cpp
    src/substitutor.cpp
    src/template.cpp
    src/template_info.cpp
    src/utils.cpp
)

set(CMAKE_EXPORT_COMPILE_COMMANDS YES)

add_executable(lppm ${LPPM_SRC})
target_include_directories(lppm PRIVATE include/)
set_property(TARGET lppm PROPERTY CXX_STANDARD 23)
target_compile_options(lppm PRIVATE -Wall -Wextra -Werror)
install(TARGETS lppm DESTINATION bin)
