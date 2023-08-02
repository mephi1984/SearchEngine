#pragma once 
#include <string>
#include <iostream>

class IniParser
{
protected:

	std::string startPage_;
	int level_;
	std::string dbHost_;
	std::string dbPort_;
	std::string dbName_;
	std::string dbUser_;
	std::string dbPassword_;
	int port_;

public:

	IniParser(const std::string& fileName);

	std::string getStartPage() const;

	int getLevel() const;

	std::string getDbHost() const;

	std::string getDbPort() const;

	std::string getDbName() const;

	std::string getDbUser() const;

	std::string getDbPassword() const;

	int getPort() const;
};