set(SRCS
            net_core.cpp
            local.cpp
            )

set(HEADERS
            net_core.h
            local.h
            )


create_directory_groups(${SRCS} ${HEADERS})

add_library(net_core STATIC ${SRCS} ${HEADERS})
