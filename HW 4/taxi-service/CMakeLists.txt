cmake_minimum_required(VERSION 3.14)
project(taxi-service CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable MongoDB support, disable other features we don't need
set(USERVER_FEATURE_MONGODB ON CACHE BOOL "" FORCE)
set(USERVER_FEATURE_POSTGRESQL OFF CACHE BOOL "" FORCE)
set(USERVER_FEATURE_REDIS OFF CACHE BOOL "" FORCE)
set(USERVER_FEATURE_CLICKHOUSE OFF CACHE BOOL "" FORCE)
set(USERVER_FEATURE_GRPC OFF CACHE BOOL "" FORCE)
set(USERVER_FEATURE_RABBITMQ OFF CACHE BOOL "" FORCE)
set(USERVER_FEATURE_MYSQL OFF CACHE BOOL "" FORCE)
set(USERVER_FEATURE_ROCKS OFF CACHE BOOL "" FORCE)
set(USERVER_FEATURE_YDB OFF CACHE BOOL "" FORCE)
set(USERVER_FEATURE_TESTSUITE OFF CACHE BOOL "" FORCE)
set(USERVER_FEATURE_FORMAT OFF CACHE BOOL "" FORCE)
set(USERVER_FEATURE_CHAOTIC OFF CACHE BOOL "" FORCE)

# Try to find preinstalled userver first, otherwise fetch from source
find_package(userver QUIET COMPONENTS core mongodb)
if(NOT userver_FOUND)
    message(STATUS "userver not found, fetching from source via CPM...")
    include(FetchContent)
    FetchContent_Declare(
        userver
        GIT_REPOSITORY https://github.com/userver-framework/userver.git
        GIT_TAG v2.6
        GIT_SHALLOW ON
    )
    FetchContent_MakeAvailable(userver)
endif()

# Find MongoDB C driver using pkg-config
include(FindPkgConfig)
pkg_check_modules(MONGOC REQUIRED libmongoc-1.0)
pkg_check_modules(BSON REQUIRED libbson-1.0)
pkg_check_modules(MONGOCXX REQUIRED libmongocxx)
pkg_check_modules(BSONCXX REQUIRED libbsoncxx)

# MongoDB C++ driver paths
set(MONGOCXX_INCLUDE_DIRS /usr/local/include/mongocxx/v_noabi)
set(BSONCXX_INCLUDE_DIRS /usr/local/include/bsoncxx/v_noabi)

message(STATUS "MongoDB libraries: ${MONGOCXX_LIBRARIES}")
message(STATUS "MongoDB include dirs: ${MONGOCXX_INCLUDE_DIRS}")

# JWT-CPP is header-only, we just need to include it
# It will be installed in /usr/local/include/jwt-cpp
message(STATUS "JWT-CPP will be used as header-only library")

set(COMMON_SOURCES
    src/common/models.cpp
    src/common/database.cpp
    src/auth/jwt_auth_checker.cpp
    src/auth/jwt_auth_factory.cpp
)

# Input Service executable
add_executable(${PROJECT_NAME}-input
    src/input/main.cpp
    src/input/handlers.cpp
    ${COMMON_SOURCES}
)

target_include_directories(${PROJECT_NAME}-input PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${MONGOCXX_INCLUDE_DIRS}
    ${BSONCXX_INCLUDE_DIRS}
    ${MONGOC_INCLUDE_DIRS}
    ${BSON_INCLUDE_DIRS}
)

target_link_libraries(${PROJECT_NAME}-input PRIVATE
    userver::core
    ${MONGOCXX_LIBRARIES}
    ${BSONCXX_LIBRARIES}
    ${MONGOC_LIBRARIES}
    ${BSON_LIBRARIES}
)

# Order Service executable
add_executable(${PROJECT_NAME}-order
    src/order/main.cpp
    src/order/handlers.cpp
    ${COMMON_SOURCES}
)

target_include_directories(${PROJECT_NAME}-order PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${MONGOCXX_INCLUDE_DIRS}
    ${BSONCXX_INCLUDE_DIRS}
    ${MONGOC_INCLUDE_DIRS}
    ${BSON_INCLUDE_DIRS}
)

target_link_libraries(${PROJECT_NAME}-order PRIVATE
    userver::core
    ${MONGOCXX_LIBRARIES}
    ${BSONCXX_LIBRARIES}
    ${MONGOC_LIBRARIES}
    ${BSON_LIBRARIES}
)

# Match Service executable
add_executable(${PROJECT_NAME}-match
    src/match/main.cpp
    src/match/handlers.cpp
    ${COMMON_SOURCES}
)

target_include_directories(${PROJECT_NAME}-match PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${MONGOCXX_INCLUDE_DIRS}
    ${BSONCXX_INCLUDE_DIRS}
    ${MONGOC_INCLUDE_DIRS}
    ${BSON_INCLUDE_DIRS}
)

target_link_libraries(${PROJECT_NAME}-match PRIVATE
    userver::core
    ${MONGOCXX_LIBRARIES}
    ${BSONCXX_LIBRARIES}
    ${MONGOC_LIBRARIES}
    ${BSON_LIBRARIES}
)

# Install targets
install(TARGETS
    ${PROJECT_NAME}-input
    ${PROJECT_NAME}-order
    ${PROJECT_NAME}-match
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    COMPONENT ${PROJECT_NAME}
)

# Install configs
install(DIRECTORY configs/
    DESTINATION ${CMAKE_INSTALL_SYSCONFDIR}/${PROJECT_NAME}
    COMPONENT ${PROJECT_NAME}
)
