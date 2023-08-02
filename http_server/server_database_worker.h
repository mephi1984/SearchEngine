#pragma once
#include "../common/database_worker.h"

class ServerDatabaseWorker : public DatabaseWorker
{
public:
	ServerDatabaseWorker(const IniParser& iniParser);

	std::vector<std::string> searchPages(const std::string& query);
};

