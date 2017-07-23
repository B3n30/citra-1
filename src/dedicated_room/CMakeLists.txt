set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR}/CMakeModules)

set(SRCS
            main.cpp
            )

set(HEADERS
            main.h
            )

add_executable(citra-room ${SRCS} ${HEADERS})
target_link_libraries(citra-room PRIVATE common core network)

if(UNIX AND NOT APPLE)
    install(TARGETS citra-room RUNTIME DESTINATION "${CMAKE_INSTALL_PREFIX}/bin")
endif()
