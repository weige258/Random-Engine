add_library(Libs INTERFACE)

#SDL
add_subdirectory(SDL)

#Vulkan
set(ENV{VULKAN_SDK} "${CMAKE_CURRENT_SOURCE_DIR}/VulkanSDK")
find_package(Vulkan REQUIRED)

# Boost库跨平台编译
set(BOOST_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/Boost")
set(BOOST_STAGE_LIB "${BOOST_ROOT}/stage/lib")

if(WIN32)
    set(BOOTSTRAP_CMD "${BOOST_ROOT}/bootstrap.bat")
    set(B2_EXE "${BOOST_ROOT}/b2.exe")
    set(BOOST_TOOLSET "toolset=clang")
    set(LIB_PATTERN "*.lib")
else()
    set(BOOTSTRAP_CMD "${BOOST_ROOT}/bootstrap.sh")
    set(B2_EXE "${BOOST_ROOT}/b2")
    set(BOOST_TOOLSET "") 
    set(LIB_PATTERN "*.a" "*.so" "*.dylib")
endif()

file(GLOB BOOST_COMPILED_LIBS "${BOOST_STAGE_LIB}/${LIB_PATTERN}")

if(NOT BOOST_COMPILED_LIBS)
    message(STATUS "[Boost] 未检测到已编译的 Boost 库，启动自动编译流程...")

    if(NOT EXISTS "${B2_EXE}")
        message(STATUS "[Boost] 正在生成 b2 构建工具...")
        execute_process(
            COMMAND ${BOOTSTRAP_CMD}
            WORKING_DIRECTORY ${BOOST_ROOT}
            RESULT_VARIABLE BOOTSTRAP_RES
        )
        if(NOT BOOTSTRAP_RES EQUAL 0)
            message(FATAL_ERROR "[Boost] bootstrap 运行失败！")
        endif()
    endif()

    message(STATUS "[Boost] 正在编译所需组件 (json, system)...")
    execute_process(
        COMMAND ${B2_EXE} ${BOOST_TOOLSET} architecture=x86 address-model=64 link=static --with-json --with-system stage
        WORKING_DIRECTORY ${BOOST_ROOT}
        RESULT_VARIABLE B2_RES
    )
    if(NOT B2_RES EQUAL 0)
        message(FATAL_ERROR "[Boost] b2 编译失败！")
    endif()
    
    message(STATUS "[Boost] 自动构建完成！")
endif()

set(BOOST_INCLUDEDIR "${BOOST_ROOT}")
set(BOOST_LIBRARYDIR "${BOOST_STAGE_LIB}")

if(WIN32)
    set(Boost_COMPILER "-clang21")
endif()

set(Boost_DIR "${BOOST_STAGE_LIB}/cmake/Boost-1.92.0")
find_package(Boost CONFIG REQUIRED COMPONENTS json system)

target_link_libraries(Libs 
    INTERFACE SDL3::SDL3
    INTERFACE Vulkan::Vulkan
    INTERFACE Boost::headers
    INTERFACE Boost::json
    INTERFACE Boost::system

)

