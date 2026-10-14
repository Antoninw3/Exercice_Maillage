cmake_minimum_required(VERSION 3.16)
project(maillage LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

find_package(OpenMesh REQUIRED)

add_library(maillage STATIC
  src/aretes.cpp
  src/allegement.cpp
  src/bords.cpp
  src/epaississement.cpp
  src/decoupeur.cpp
  src/reparateur.cpp
  src/triangulation.cpp
)
target_include_directories(maillage PUBLIC include)
target_link_libraries(maillage PUBLIC OpenMeshCore OpenMeshTools)
target_compile_options(maillage PRIVATE -Wall -Wextra)

add_executable(objread src/main.cpp)
target_link_libraries(objread PRIVATE maillage)
target_compile_options(objread PRIVATE -Wall -Wextra)

add_executable(grille outils/grille.cpp)
target_compile_options(grille PRIVATE -Wall -Wextra)

add_executable(flottant outils/flottant.cpp)
target_compile_options(flottant PRIVATE -Wall -Wextra)
