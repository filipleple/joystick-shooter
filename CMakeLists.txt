cmake_minimum_required(VERSION 3.0.0)
project(joysticc3 VERSION 0.1.0)

include(CTest)
enable_testing()

add_executable(joysticc3 main.cpp)
target_link_libraries(joysticc3 SDL2 SDL2main SDL2_ttf GL)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
