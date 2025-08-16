cmake_minimum_required(VERSION 3.18)

project(game_server CXX)
set(CMAKE_CXX_STANDARD 20)

include(${CMAKE_BINARY_DIR}/conanbuildinfo_multi.cmake)
conan_basic_setup(TARGETS)

get_property(importTargets DIRECTORY "${CMAKE_SOURCE_DIR}" PROPERTY IMPORTED_TARGETS)
message(STATUS "${importTargets}")

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

# Добавляем библиотеку, указывая, что она статическая.
add_library(GameModelAndAppLib STATIC
	src/model.h
	src/model.cpp
	src/tagged.h
	src/extra_data.h
	src/extra_data.cpp
	src/application.h
	src/application.cpp
	src/loot_generator.cpp
	src/loot_generator.h
	src/geom.h
	src/collision_detector.h
	src/collision_detector.cpp
	src/model_serialization.h
	src/model_serialization.cpp
	src/serialization_listener.h
	src/serialization_listener.cpp
	src/database/app/use_cases.h
	src/database/app/use_cases_impl.h
	src/database/app/use_cases_impl.cpp
	src/database/domain/player_repository_fwd.h
	src/database/domain/player_repository.h
	src/database/postgres/connection_pool.h
	src/database/postgres/postgres.h
	src/database/postgres/postgres.cpp
)

# Добавляем сторонние библиотеки. Указываем видимость PUBLIC, т. к. 
# они должны быть ввидны и в библиотеке MyLib и в зависимостях.
target_link_libraries(GameModelAndAppLib PUBLIC CONAN_PKG::boost CONAN_PKG::libpq CONAN_PKG::libpqxx)

add_executable(game_server
	src/main.cpp
	src/http_server.cpp
	src/http_server.h
	src/sdk.h
	src/boost_json.cpp
	src/json_loader.h
	src/json_loader.cpp
	src/request_handler.h
	src/logger.h
	src/logger.cpp
	src/response_utils.h
	src/response_utils.cpp
	src/file_request_handler.h
	src/file_request_handler.cpp
	src/api_request_handler.h
	src/api_request_handler.cpp
	src/ticker.h
	src/ticker.cpp
	src/programm_options.h
	src/programm_options.cpp
)

target_link_libraries(game_server PRIVATE Threads::Threads CONAN_PKG::boost CONAN_PKG::libpq CONAN_PKG::libpqxx GameModelAndAppLib)

add_executable(game_server_tests
	tests/loot_generator_tests.cpp
	tests/collision-detector-tests.cpp
	src/json_loader.h
	src/json_loader.cpp
	tests/state-serialization-tests.cpp
	
)

target_link_libraries(game_server_tests PRIVATE CONAN_PKG::catch2 GameModelAndAppLib)

include(CTest)
include(${CONAN_BUILD_DIRS_CATCH2_DEBUG}/Catch.cmake)

catch_discover_tests(game_server_tests)
