#pragma once
#include <mutex>
#include <pqxx/pqxx>
#include "ini_parser.h"

class DatabaseWorker
{
protected:
	pqxx::connection c;

public:
	DatabaseWorker(const IniParser& iniParser);
};









