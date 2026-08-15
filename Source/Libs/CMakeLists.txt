add_library(Libs INTERFACE)

add_subdirectory(glm)
add_subdirectory(SDL)

set(ENV{VULKAN_SDK} "${CMAKE_CURRENT_SOURCE_DIR}/VulkanSDK")
find_package(Vulkan REQUIRED)

target_link_libraries(Libs 
    INTERFACE glm
    INTERFACE SDL3::SDL3
    INTERFACE Vulkan::Vulkan
    )
