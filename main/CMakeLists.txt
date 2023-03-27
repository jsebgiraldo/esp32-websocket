
file(GLOB srcs
        "*.c"
        "APIs/HTTP_SERVER/*.c"
        "APIs/WIFI_API/*.c"
        "APIs/NVS/*.c"
        )

set(dirs
        "APIs/HTTP_SERVER"
        "APIs/WIFI_API"
        "APIs/NVS"
        )


idf_component_register(
        SRCS ${srcs}
        INCLUDE_DIRS ${dirs}
        )