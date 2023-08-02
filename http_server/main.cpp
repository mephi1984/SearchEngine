#include <chrono>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <string>

#include <string>
#include <iostream>

#include "../common/common_functions.h"
#include "../common/ini_parser.h"
#include "server_database_worker.h"

#include "http_connection.h"
#include <Windows.h>


void httpServer(tcp::acceptor& acceptor, tcp::socket& socket, ServerDatabaseWorker& databaseWorker)
{
	acceptor.async_accept(socket,
		[&](beast::error_code ec)
		{
			if (!ec)
				std::make_shared<HttpConnection>(std::move(socket), databaseWorker)->start();
			httpServer(acceptor, socket, databaseWorker);
		});
}


int main(int argc, char* argv[])
{
	SetConsoleCP(CP_UTF8);
	SetConsoleOutputCP(CP_UTF8);

	try
	{
		IniParser iniParser("C:\\Work\\Projects\\DiplomProject001\\settings.ini");

		ServerDatabaseWorker databaseWorker(iniParser);

		auto const address = net::ip::make_address("0.0.0.0");
		unsigned short port = 8080;

		net::io_context ioc{1};

		tcp::acceptor acceptor{ioc, { address, port }};
		tcp::socket socket{ioc};
		httpServer(acceptor, socket, databaseWorker);

		ioc.run();
	}
	catch (std::exception const& e)
	{
		std::cerr << "Error: " << e.what() << std::endl;
		return EXIT_FAILURE;
	}
}