cmake_minimum_required(VERSION 3.16)
project(maillage LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

find_package(OpenMesh REQUIRED)

add_executable(objread
  src/main.cpp
  src/bords.cpp
  src/epaississement.cpp
  src/decoupeur.cpp
  src/reparateur.cpp
)
target_include_directories(objread PRIVATE include)
target_link_libraries(objread PRIVATE OpenMeshCore OpenMeshTools)
target_compile_options(objread PRIVATE -Wall -Wextra)
