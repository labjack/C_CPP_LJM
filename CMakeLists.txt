cmake_minimum_required(VERSION 3.12)
project(C_CPP_LJM C)

enable_testing()

if(WIN32)
    include_directories("C:/Program Files (x86)/LabJack/Drivers")
    link_directories("C:/Program Files (x86)/LabJack/Drivers/64bit")
    link_directories("C:/Windows/System32")
elseif(Darwin)
    include_directories("/usr/local/include")
    link_directories("/usr/local/lib")
elseif(Linux)
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
    
    target_link_libraries(${FULL_NAME} PUBLIC LabJackM)

    add_test(NAME ${FULL_NAME} COMMAND ${FULL_NAME})

    install(TARGETS ${FULL_NAME} RUNTIME DESTINATION ${DIRECTORY})

  endforeach()
endforeach()