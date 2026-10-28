# Build the light sampler application, using the HAL

include_directories(include)
file(GLOB MY_SOURCES "src/*.c")

add_executable(light_sampler ${MY_SOURCES})

target_link_libraries(light_sampler LINK_PRIVATE hal)

add_custom_command(TARGET light_sampler POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E copy
        "$<TARGET_FILE:light_sampler>"
        "/home/madden/ensc351/public/myApps/light_sampler"
    COMMENT "Copying executable to public NFS directory"
)
