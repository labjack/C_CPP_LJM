cmake_minimum_required(VERSION 3.12)

set(CMAKE_INSTALL_PREFIX ${CMAKE_SOURCE_DIR}/install CACHE PATH "Install Path")

project(C_CPP_LJM C)

enable_testing()

if(CMAKE_SYSTEM MATCHES Windows)
    include_directories("C:/Program Files (x86)/LabJack/Drivers")
    link_directories("C:/Program Files (x86)/LabJack/Drivers/64bit")
    link_directories("C:/Windows/System32")
elseif(CMAKE_SYSTEM MATCHES Darwin)
    include_directories("/usr/local/include")
    link_directories("/usr/local/lib")
elseif(CMAKE_SYSTEM MATCHES Linux)
    include_directories("/usr/local/include")
    link_directories("/usr/local/lib")
endif()

set(SRC_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}/basic
    ${CMAKE_CURRENT_SOURCE_DIR}/more
)

foreach(DIR IN LISTS SRC_DIRS)
  file(GLOB_RECURSE SRC_PATHS
       RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
       LIST_DIRECTORIES false
       CONFIGURE_DEPENDS
       ${DIR}/*.c
  )

  foreach(SRC IN LISTS SRC_PATHS)
    get_filename_component(NAME ${SRC} NAME_WE)
    get_filename_component(PATH ${SRC} DIRECTORY)

    string(REPLACE "/" "_" FULL_NAME "${PATH}/${NAME}")

    add_executable(${FULL_NAME} ${SRC})

    if(NOT "dynamic_runtime_linking" STREQUAL ${NAME}) 
      target_link_libraries(${FULL_NAME} PUBLIC LabJackM)
    else()
      # Linking for dynamic_runtime_linking example.
      if(NOT WIN32)
        target_link_libraries(${FULL_NAME} PUBLIC dl)
      endif()
    endif()

    add_test(NAME ${FULL_NAME} COMMAND ${FULL_NAME})

    install(TARGETS ${FULL_NAME} RUNTIME DESTINATION ${DIRECTORY})
  endforeach()
endforeach()