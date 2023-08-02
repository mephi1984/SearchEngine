cmake_minimum_required(VERSION 3.20)

project(CppDiplomProject)



# Define by your own needs:
set(OPENSSL_ROOT_DIR "C:\\Program Files\\OpenSSL-Win64")

set(BOOST_ROOT "C:\\Boost")

set(libpqxx_DIR "C:\\Work\\Projects\\libpqxx-7.7.5")

find_package(Boost 1.80.0 REQUIRED COMPONENTS system thread locale)

if(NOT Boost_FOUND)
    message(FATAL_ERROR "Could not find Boost")
endif()




add_executable(SpiderApp
	spider/main.cpp
	spider/parser_database_worker.h
	spider/parser_database_worker.cpp
	spider/http_utils.h
	spider/http_utils.cpp
	spider/link.h
	common/common.h
	common/common.cpp
	common/database_worker.h
	common/database_worker.cpp
	common/ini_parser.h
	common/ini_parser.cpp
	)


set(PostgreSQL_ADDITIONAL_VERSIONS "15")

add_subdirectory(${libpqxx_DIR} libpqxx-build) 

target_compile_features(SpiderApp PRIVATE cxx_std_17) 

target_link_libraries(SpiderApp pqxx)





target_include_directories(SpiderApp PRIVATE ${Boost_INCLUDE_DIRS})

target_link_libraries(SpiderApp ${Boost_LIBRARIES})

find_package(OpenSSL REQUIRED)

target_link_libraries(SpiderApp OpenSSL::SSL)

add_executable(HttpServerApp 
	http_server/search.cpp
	http_server/server_database_worker.h
	http_server/server_database_worker.cpp
	http_server/http_connection.h
	http_server/http_connection.cpp
	common/common.h
	common/common.cpp
	common/database_worker.h
	common/database_worker.cpp
	common/ini_parser.h
	common/ini_parser.cpp
	)

target_compile_features(HttpServerApp PRIVATE cxx_std_17) 

target_link_libraries(HttpServerApp pqxx)  

target_include_directories(HttpServerApp PRIVATE ${Boost_INCLUDE_DIRS})

target_link_libraries(HttpServerApp ${Boost_LIBRARIES})

target_link_libraries(HttpServerApp OpenSSL::SSL)
