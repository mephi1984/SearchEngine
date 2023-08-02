#include "ini_parser.h"

#include <fstream>
#include <map>


IniParser::IniParser(const std::string& fileName)
	{
		std::ifstream ini_file(fileName);

		if (!ini_file.is_open())
		{
			throw std::runtime_error("File settings.ini not found");
		}

		std::map<std::string, std::string> my_map;

		std::string line;
		while (std::getline(ini_file, line)) {

			if ((line.size() > 0) && (line[0] == ';'))
			{
				continue;
			}

			size_t commentPos = line.find(" ;");

			std::string lineBeforeComment = line.substr(0, commentPos);

			size_t pos = lineBeforeComment.find('=');
			if (pos == std::string::npos)
				continue;

			std::string key = lineBeforeComment.substr(0, pos);
			std::string value = lineBeforeComment.substr(pos + 1);
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

		startPage_ = my_map["start_page"];

		level_ = std::stoi(my_map["level"]);

		if (level_ < 0)
		{
			throw std::runtime_error("Level must be 0 or greater.");
		}

		dbHost_ = my_map["db_host"];
		dbPort_ = my_map["db_port"];
		dbName_ = my_map["db_name"];
		dbUser_ = my_map["db_user"];
		dbPassword_ = my_map["db_password"];

		port_ = std::stoi(my_map["port"]);

		if ((port_ <= 0) || (port_ > 65535))
		{
			throw std::runtime_error("Port must be in range from 1 to 65535 or greater.");
		}


	}

	std::string IniParser::getStartPage() const
	{
		return startPage_;
	}

	int IniParser::getLevel() const
	{
		return level_;
	}

	std::string IniParser::getDbHost() const
	{
		return dbHost_;
	}
	 
	std::string IniParser::getDbPort() const
	{
		return dbPort_;
	}

	std::string IniParser::getDbName() const
	{
		return dbName_;
	}

	std::string IniParser::getDbUser() const
	{
		return dbUser_;
	}

	std::string IniParser::getDbPassword() const
	{
		return dbPassword_;
	}

	int IniParser::getPort() const
	{
		return port_;
	}
