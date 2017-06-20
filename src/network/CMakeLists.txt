set(SRCS
            network.cpp
            room.cpp
            room_member.cpp
            )

set(HEADERS
            network.h
            room.h
            room_member.h
            )

include_directories(../../externals/enet/include)

create_directory_groups(${SRCS} ${HEADERS})

add_library(network STATIC ${SRCS} ${HEADERS})
target_link_libraries(network enet)
