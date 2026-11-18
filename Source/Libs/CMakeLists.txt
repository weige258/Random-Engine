add_library(Libs INTERFACE)

# SDL 只构建静态库，不构建 DLL。
# 原因：SDL 的 version.rc 只加入 shared 目标（sdl_glob_sources(SHARED "*.rc")），
# 而 MinGW 的 windres 在把 -I 参数转交 cc1 预处理时会丢失引号，
# 导致工程路径中的空格（"Random Engine"）被劈成两半，预处理直接失败。
# 改为静态库即可完全绕开 windres；SDL3::SDL3 别名会自动指向 SDL3-static。
# 注意：这两个开关必须在 add_subdirectory(SDL) 之前设置，否则对本次配置无效。
set(SDL_SHARED OFF CACHE BOOL "" FORCE)
set(SDL_STATIC ON  CACHE BOOL "" FORCE)

#SDL
add_subdirectory(SDL)

# 让 SDL 不参与 LTO：它是第三方静态库，跨模块内联收益很小，但链接期要为它跑大量
# LTRANS 任务（MinGW 上是串行的），是 Release 每次链接慢的主要原因。
# 只在 GCC 下加，MSVC/Clang 完全不受影响（MSVC 用 /GL，且不认 -fno-lto）。
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND TARGET SDL3-static)
    target_compile_options(SDL3-static PRIVATE
        $<$<CONFIG:Release,RelWithDebInfo,MinSizeRel>:-fno-lto>)
endif()


#Vulkan
set(ENV{VULKAN_SDK} "${CMAKE_CURRENT_SOURCE_DIR}/VulkanSDK")
find_package(Vulkan REQUIRED)

target_link_libraries(Libs 
    INTERFACE SDL3::SDL3
    INTERFACE Vulkan::Vulkan
)