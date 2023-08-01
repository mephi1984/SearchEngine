#pragma once 
#include <string>
#include <iostream>
#include <fstream>
#include <map>

class IniParser
{
protected:

	std::string start_page;
	int level;
	std::string db_host;
	std::string db_port;
	std::string db_name;
	std::string db_user;
	std::string db_password;
	int port;

public:

	IniParser(const std::string& fileName)
	{
		std::ifstream ini_file(fileName);

		if (!ini_file.is_open())
		{
			throw std::runtime_error("File settings.ini not found");
		}

		std::map<std::string, std::string> my_map;

		std::string line;
		while (std::getline(ini_file, line)) {
			size_t pos = line.find('=');
			if (pos == std::string::npos)
				continue;

			std::string key = line.substr(0, pos);
			std::string value = line.substr(pos + 1);
			my_map[key] = value;
		}

		if (my_map.count("start_page") == 0)
		{
			throw std::runtime_error("Please define start_page in the settings.ini file.");
		}
		if (my_map.count("level") == 0)
		{
			throw std::runtime_error("Please define level in the settings.ini file.");
		}
		if (my_map.count("db_host") == 0)
		{
			throw std::runtime_error("Please define db_host in the settings.ini file.");
		}
		if (my_map.count("db_port") == 0)
		{
			throw std::runtime_error("Please define db_host in the settings.ini file.");
		}
		if (my_map.count("db_name") == 0)
		{
			throw std::runtime_error("Please define db_name in the settings.ini file.");

		}
		if (my_map.count("db_user") == 0)
		{
			throw std::runtime_error("Please define db_user in the settings.ini file.");

		}
		if (my_map.count("db_password") == 0)
		{
			throw std::runtime_error("Please define db_password in the settings.ini file.");
		}
		if (my_map.count("port") == 0)
		{
			throw std::runtime_error("Please define port in the settings.ini file.");
		}

		start_page = my_map["start_page"];

		level = std::stoi(my_map["level"]);

		if (level < 0)
		{
			throw std::runtime_error("Level must be 0 or greater.");
		}

		db_host = my_map["db_host"];
		db_port = my_map["db_port"];
		db_name = my_map["db_name"];
		db_user = my_map["db_user"];
		db_password = my_map["db_password"];

		port = std::stoi(my_map["port"]);

		if ((port <= 0) || (port > 65535))
		{
			throw std::runtime_error("Port must be in range from 1 to 65535 or greater.");
		}


	}

	std::string getStartPage() const
	{
		return start_page;
	}

	int getLevel() const
	{
		return level;
	}

	std::string getDbHost() const
	{
		return db_host;
	}
	 
	std::string getDbPort() const
	{
		return db_port;
	}

	std::string getDbName() const
	{
		return db_name;
	}

	std::string getDbUser() const
	{
		return db_user;
	}

	std::string getDbPassword() const
	{
		return db_password;
	}

	int getPort() const
	{
		return port;
	}
};