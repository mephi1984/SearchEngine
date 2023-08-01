cmake_minimum_required(VERSION 3.20)

set(PostgreSQL_ADDITIONAL_VERSIONS "15")

project(DiplomProject001)

add_executable(DiplomProject001 
	main.cpp
	database_worker.h
	database_worker.cpp
	http_utils.h
	http_utils.cpp
	common.h
	common.cpp
	ini_parser.h
	link.h
	)

add_subdirectory("C:/Work/Projects/libpqxx-7.7.4" libpqxx-build) 

target_compile_features(DiplomProject001 PRIVATE cxx_std_17) 

target_link_libraries(DiplomProject001 pqxx)              

set(Boost_ADDITIONAL_VERSIONS 1.80.0 1.80)

set(BOOST_ROOT "C:\\Work\\Projects\\boost_1_80_0")

find_package(Boost 1.80.0 REQUIRED COMPONENTS system thread locale)

if(NOT Boost_FOUND)
    message(FATAL_ERROR "Could not find Boost")
endif()

target_include_directories(DiplomProject001 PRIVATE ${Boost_INCLUDE_DIRS})

target_link_libraries(DiplomProject001 ${Boost_LIBRARIES})

set(OPENSSL_ROOT_DIR "C:\\Program Files\\OpenSSL-Win64")

find_package(OpenSSL REQUIRED)

target_link_libraries(DiplomProject001 OpenSSL::SSL)

add_executable(SearchEngine search.cpp ini_parser.h)

target_compile_features(SearchEngine PRIVATE cxx_std_17) 

target_link_libraries(SearchEngine pqxx)  

target_include_directories(SearchEngine PRIVATE ${Boost_INCLUDE_DIRS})

target_link_libraries(SearchEngine ${Boost_LIBRARIES})

target_link_libraries(SearchEngine OpenSSL::SSL)
