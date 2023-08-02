cmake_minimum_required(VERSION 3.20)


add_executable(SpiderApp
	main.cpp
	parser_database_worker.h
	parser_database_worker.cpp
	http_utils.h
	http_utils.cpp
	link.h
	../common/common_functions.h
	../common/common_functions.cpp
	../common/database_worker.h
	../common/database_worker.cpp
	../common/ini_parser.h
	../common/ini_parser.cpp
	)

target_compile_features(SpiderApp PRIVATE cxx_std_17) 

target_link_libraries(SpiderApp pqxx)

target_link_libraries(SpiderApp OpenSSL::SSL)

target_include_directories(SpiderApp PRIVATE ${Boost_INCLUDE_DIRS})

target_link_libraries(SpiderApp ${Boost_LIBRARIES})
