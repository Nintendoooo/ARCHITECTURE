cmake_minimum_required(VERSION 3.12)
project(taxi-service CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(userver REQUIRED COMPONENTS core)

find_package(SQLite3 REQUIRED)
if(NOT SQLite3_FOUND)
    message(FATAL_ERROR "SQLite3 not found")
endif()

message(STATUS "SQLite3 libraries: ${SQLITE3_LIBRARIES}")
message(STATUS "SQLite3 include dirs: ${SQLITE3_INCLUDE_DIRS}")

message(STATUS "JWT-CPP will be used as header-only library")

set(COMMON_SOURCES
    src/common/models.cpp
    src/common/database.cpp
    src/auth/jwt_auth_checker.cpp
    src/auth/jwt_auth_factory.cpp
)

add_executable(${PROJECT_NAME}-input
    src/input/main.cpp
    src/input/handlers.cpp
    ${COMMON_SOURCES}
)

target_include_directories(${PROJECT_NAME}-input PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${SQLITE3_INCLUDE_DIRS}
)

target_link_libraries(${PROJECT_NAME}-input PRIVATE
    userver::core
    /usr/lib/x86_64-linux-gnu/libsqlite3.so
)

add_executable(${PROJECT_NAME}-order
    src/order/main.cpp
    src/order/handlers.cpp
    ${COMMON_SOURCES}
)

target_include_directories(${PROJECT_NAME}-order PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${SQLITE3_INCLUDE_DIRS}
)

target_link_libraries(${PROJECT_NAME}-order PRIVATE
    userver::core
    /usr/lib/x86_64-linux-gnu/libsqlite3.so
)

add_executable(${PROJECT_NAME}-match
    src/match/main.cpp
    src/match/handlers.cpp
    ${COMMON_SOURCES}
)

target_include_directories(${PROJECT_NAME}-match PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${SQLITE3_INCLUDE_DIRS}
)

target_link_libraries(${PROJECT_NAME}-match PRIVATE
    userver::core
    /usr/lib/x86_64-linux-gnu/libsqlite3.so
)

install(TARGETS
    ${PROJECT_NAME}-input
    ${PROJECT_NAME}-order
    ${PROJECT_NAME}-match
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    COMPONENT ${PROJECT_NAME}
)

install(DIRECTORY configs/
    DESTINATION ${CMAKE_INSTALL_SYSCONFDIR}/${PROJECT_NAME}
    COMPONENT ${PROJECT_NAME}
)
