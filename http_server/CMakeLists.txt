cmake_minimum_required(VERSION 3.20)


add_executable(HttpServerApp 
	main.cpp
	server_database_worker.h
	server_database_worker.cpp
	http_connection.h
	http_connection.cpp
	../common/common_functions.h
	../common/common_functions.cpp
	../common/database_worker.h
	../common/database_worker.cpp
	../common/ini_parser.h
	../common/ini_parser.cpp
	)

target_compile_features(HttpServerApp PRIVATE cxx_std_17) 

target_link_libraries(HttpServerApp pqxx)  

target_include_directories(HttpServerApp PRIVATE ${Boost_INCLUDE_DIRS})

target_link_libraries(HttpServerApp ${Boost_LIBRARIES})

target_link_libraries(HttpServerApp OpenSSL::SSL)

