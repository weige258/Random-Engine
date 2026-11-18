add_library(Libs INTERFACE)

#SDL
add_subdirectory(SDL)
# SDL 只构建静态库，不构建 DLL。
# 原因：SDL 的 version.rc 只加入 shared 目标（sdl_glob_sources(SHARED "*.rc")），
# 而 MinGW 的 windres 在把 -I 参数转交 cc1 预处理时会丢失引号，
# 导致工程路径中的空格（"Random Engine"）被劈成两半，预处理直接失败。
# 改为静态库即可完全绕开 windres；SDL3::SDL3 别名会自动指向 SDL3-static。
set(SDL_SHARED OFF CACHE BOOL "" FORCE)
set(SDL_STATIC ON  CACHE BOOL "" FORCE)


#Vulkan
set(ENV{VULKAN_SDK} "${CMAKE_CURRENT_SOURCE_DIR}/VulkanSDK")
find_package(Vulkan REQUIRED)

target_link_libraries(Libs 
    INTERFACE SDL3::SDL3
    INTERFACE Vulkan::Vulkan
)

