#include "database_worker.h"
#include "http_utils.h"
#include "common.h"


DatabaseWorker::DatabaseWorker(const IniParser& iniParser)
		: c("host=" + iniParser.getDbHost() +
			" port=" + iniParser.getDbPort() +
			" dbname=" + iniParser.getDbName() +
			" user=" + iniParser.getDbUser() +
			" password=" + iniParser.getDbPassword())
	{
	}
