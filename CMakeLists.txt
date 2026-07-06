cmake_minimum_required(VERSION 3.16)
project(DualProcRpc VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Network WebSockets Widgets)

# === Shared library ===
add_library(rpc_shared STATIC
    shared/Protocol.h
    shared/Protocol.cpp
)
target_include_directories(rpc_shared PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(rpc_shared PUBLIC Qt6::Core)

# === Program A (Caller) ===
add_executable(ProgramA
    ProgramA/main.cpp
    ProgramA/MainWindow.h
    ProgramA/MainWindow.cpp
    ProgramA/HttpClient.h
    ProgramA/HttpClient.cpp
    ProgramA/WebSocketClient.h
    ProgramA/WebSocketClient.cpp
)
target_link_libraries(ProgramA PRIVATE rpc_shared Qt6::Core Qt6::Network Qt6::WebSockets Qt6::Widgets)

# === Program B (Executor) ===
add_executable(ProgramB
    ProgramB/main.cpp
    ProgramB/MainWindow.h
    ProgramB/MainWindow.cpp
    ProgramB/HttpServer.h
    ProgramB/HttpServer.cpp
    ProgramB/WebSocketServer.h
    ProgramB/WebSocketServer.cpp
    ProgramB/FunctionRegistry.h
    ProgramB/FunctionRegistry.cpp
    ProgramB/BusinessFunctions.h
    ProgramB/BusinessFunctions.cpp
)
target_link_libraries(ProgramB PRIVATE rpc_shared Qt6::Core Qt6::Network Qt6::WebSockets Qt6::Widgets)
