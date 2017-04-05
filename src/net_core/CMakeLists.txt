set(SRCS
            net_core.cpp
            )

set(HEADERS
            net_core.h
            )

include_directories(../../externals/tunnler/include)
include_directories(../../externals/tunnler/externals/RakNet/source)

create_directory_groups(${SRCS} ${HEADERS})

add_library(net_core STATIC ${SRCS} ${HEADERS})
target_link_libraries(net_core tunnler)
