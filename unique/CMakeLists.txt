set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

find_package(Abc REQUIRED)
if (ABC_FOUND)
  include_directories(${ABC_INCLUDE_DIR})
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${ABC_CXXFLAGS}")
  add_definitions (-DABC_NAMESPACE=${ABC_NAMESPACE})
else()
  set(ABC_LIBRARY "")
endif ()

find_package(Minisat REQUIRED)
if (MINISAT_FOUND)
  include_directories(${MINISAT_INCLUDE_DIR})
else()
  set (MINISAT_LIBRARY "")
endif()

find_package(Avy REQUIRED)
if (AVY_FOUND)
  include_directories(${AVY_SOURCE_DIR} ${AVY_INCLUDE_DIR})
endif ()

find_package(InterpolatingSolver REQUIRED)
if (InterpolatingSolver_FOUND)
  include_directories(${INTERPOLATING_SOLVER_INCLUDE_DIR})
endif()

add_library(extractor extractor.cc)
add_library(qbfparser QBFParser.h QBFParser.cc)
add_library(qcirparser QCIRParser.h QCIRParser.cc)
add_library(qdimacsparser QDIMACSParser.h QDIMACSParser.cc)
add_library(dqdimacsparser DQDIMACSParser.h DQDIMACSParser.cc)
add_library(dqcirparser DQCIRParser.h DQCIRParser.cc)
add_library(comparator VariableComparator.h VariableComparator.cc)

add_executable(unique determined.cc)

if (BUILD_SHARED_LIBS)
  target_link_libraries(unique docopt dqcirparser qcirparser qdimacsparser dqdimacsparser qbfparser extractor ${INTERPOLATING_SOLVER_LIBRARY} comparator)
else()
  target_link_libraries(unique docopt_s dqcirparser qcirparser qdimacsparser dqdimacsparser qbfparser extractor ${INTERPOLATING_SOLVER_LIBRARY} comparator -static)
endif()

