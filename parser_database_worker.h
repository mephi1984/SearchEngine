#pragma once
#include <mutex>
#include "database_worker.h"
#include "link.h"
class ParserDatabaseWorker : public DatabaseWorker
{
protected:
	std::mutex documentAddMutex_;

public:
	ParserDatabaseWorker(const IniParser& iniParser);

		void createTables();


		bool documentExists(const Link& link);

		void addDocumentWithWordsIfNotExists(const Link& link, const std::map<std::string, int>& wordCount);

};

