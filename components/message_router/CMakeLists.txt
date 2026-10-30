set(SOURCES "message_router.cpp")
if(CONFIG_ENABLE_SERVICE_MSGR)
    list(APPEND SOURCES "message_router_service.cpp")
endif()

idf_component_register(SRCS ${SOURCES}
                       INCLUDE_DIRS "include"
                       PRIV_REQUIRES log
                       REQUIRES interfaces 
                       ${extra_args})