cmake_minimum_required(VERSION 3.16)
project(OTL_Generator 
    VERSION 1.0.0 
    DESCRIPTION "One-Time Link Generator with Qt6 and Cloudflare"
    LANGUAGES CXX
)

# ============================================================================
# 設置 C++ 標準
# ============================================================================
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ============================================================================
# Qt6 設置
# ============================================================================
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# 查找 Qt6
find_package(Qt6 COMPONENTS 
    Core 
    Gui 
    Widgets 
    Network
    REQUIRED
)

# ============================================================================
# 查找 OpenSSL（密碼學庫）
# ============================================================================
find_package(OpenSSL REQUIRED)

# ============================================================================
# 源文件列表
# ============================================================================
set(SOURCES
    src/main.cpp
    src/mainwindow.cpp
    src/mainwindow.h
    src/cryptoengine.cpp
    src/cryptoengine.h
    src/otlgenerator.cpp
    src/otlgenerator.h
    src/linkbuilder.cpp
    src/linkbuilder.h
    src/securestring.h
)

# ============================================================================
# 創建可執行文件
# ============================================================================
add_executable(OTL_Generator WIN32 ${SOURCES})
# ============================================================================
# 設置目標屬性
# ============================================================================
target_include_directories(OTL_Generator PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# ============================================================================
# 鏈接庫
# ============================================================================
target_link_libraries(OTL_Generator PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Network
    OpenSSL::SSL
    OpenSSL::Crypto
)

# ============================================================================
# 編譯選項（MSVC）
# ============================================================================
if(MSVC)
    # 警告級別：最高
    target_compile_options(OTL_Generator PRIVATE /W4 /WX /wd4996)

    # 安全檢查
    target_compile_options(OTL_Generator PRIVATE /GS)
    
    # 運行時庫：多線程
    set_property(TARGET OTL_Generator PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    
    # 編譯優化
    target_compile_options(OTL_Generator PRIVATE $<$<CONFIG:Release>:/O2>)
endif()

# ============================================================================
# 編譯選項（GCC/Clang）
# ============================================================================
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(OTL_Generator PRIVATE 
        -Wall -Wextra -Wpedantic -Werror
        -fstack-protector-strong
    )
endif()

# ============================================================================
# 鏈接選項（Windows 安全）
# ============================================================================
if(WIN32)
    # ASLR（位置無關可執行）
    target_link_options(OTL_Generator PRIVATE /DYNAMICBASE)
    
    # DEP（數據執行保護）
    target_link_options(OTL_Generator PRIVATE /NXCOMPAT)
endif()

# ============================================================================
# 輸出目錄
# ============================================================================
set_target_properties(OTL_Generator PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
)

# ============================================================================
# 版本信息
# ============================================================================
target_compile_definitions(OTL_Generator PRIVATE
    OTL_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
    OTL_VERSION_MINOR=${PROJECT_VERSION_MINOR}
    OTL_VERSION_PATCH=${PROJECT_VERSION_PATCH}
)

# ============================================================================
# 安裝目標（可選）
# ============================================================================
install(TARGETS OTL_Generator
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

# ============================================================================
# 打印構建信息
# ============================================================================
message(STATUS "OTL Generator Build Configuration:")
message(STATUS "  C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  Compiler: ${CMAKE_CXX_COMPILER_ID}")
message(STATUS "  Platform: ${CMAKE_SYSTEM_NAME}")
message(STATUS "  OpenSSL: ${OPENSSL_VERSION}")
message(STATUS "  Build Type: ${CMAKE_BUILD_TYPE}")
