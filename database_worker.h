#pragma once
#include <mutex>
#include <pqxx/pqxx>
#include "ini_parser.h"
#include "link.h"

class DatabaseWorker
{
protected:
	pqxx::connection c;

	std::mutex document_add_mutex;

public:
	DatabaseWorker(const IniParser& iniParser);

		void createTables();


		bool documentExists(const Link& link);

		void addDocumentWithWordsIfNotExists(const Link& link, const std::vector<std::string>& words);

};









