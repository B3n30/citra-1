set(SRCS
            network.cpp
            packet.cpp
            room.cpp
            room_member.cpp
            )

set(HEADERS
            network.h
            packet.h
            room.h
            room_member.h
            )

create_directory_groups(${SRCS} ${HEADERS})

add_library(network STATIC ${SRCS} ${HEADERS})
if (ENABLE_WEB_SERVICE)
    target_link_libraries(network PUBLIC web_service)
endif()
target_link_libraries(network PRIVATE common enet)
if (ENABLE_WEB_SERVICE)
    target_link_libraries(network PUBLIC json-headers web_service)
endif()
