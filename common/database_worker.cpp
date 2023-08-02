#include "database_worker.h"


DatabaseWorker::DatabaseWorker(const IniParser& iniParser)
	: c("host=" + iniParser.getDbHost() +
		" port=" + iniParser.getDbPort() +
		" dbname=" + iniParser.getDbName() +
		" user=" + iniParser.getDbUser() +
		" password=" + iniParser.getDbPassword())
{
	createTables();
}

void DatabaseWorker::createTables()
{
	pqxx::work tx{ c };

	tx.exec("create table if not exists words (id varchar(255) not null primary key); ");
	tx.exec("create table if not exists documents (id serial primary key, protocol int not null, host text not null, query text not null); ");
	tx.exec("create table if not exists words_documents (id serial primary key, word_id varchar(255) not null references words(id), document_id integer not null references documents(id), amount integer not null); ");
	tx.commit();
}

