#include "database_worker.h"
#include "http_utils.h"
#include "common.h"

std::map<std::string, int> countWords(const std::vector<std::string>& words)
{
	std::map<std::string, int> wordCount;

	for (auto& s : words)
	{
		if ((s.size() >= 3) && (s.size() < 255))
		{
			
			auto word = wordToLowerCase(s);

			if (wordCount.count(word) == 0)
			{
				wordCount[word] = 1;
			}
			else
			{
				wordCount[word]++;
			}
		}
	}

	return wordCount;

}


DatabaseWorker::DatabaseWorker(const IniParser& iniParser)
		: c("host=" + iniParser.getDbHost() +
			" port=" + iniParser.getDbPort() +
			" dbname=" + iniParser.getDbName() +
			" user=" + iniParser.getDbUser() +
			" password=" + iniParser.getDbPassword())
	{
		createTables();

		c.prepare("insert_document", "INSERT INTO documents (protocol, host, query) VALUES ($1, $2, $3) RETURNING id;");
		c.prepare("insert_word", "INSERT INTO words (id) VALUES ($1) ON CONFLICT (id) DO NOTHING;");
		c.prepare("insert_word_for_document", "INSERT INTO words_documents (word_id, document_id, amount) VALUES ($1, $2, $3) RETURNING id;");
		c.prepare("document_count", "SELECT COUNT(*) from documents where protocol=$1 and host=$2 and query=$3");

	}

	void DatabaseWorker::createTables()
	{
		pqxx::work tx{ c };

		tx.exec("create table if not exists words (id varchar(255) not null primary key); ");
		tx.exec("create table if not exists documents (id serial primary key, protocol int not null, host text not null, query text not null); ");
		tx.exec("create table if not exists words_documents (id serial primary key, word_id varchar(255) not null references words(id), document_id integer not null references documents(id), amount integer not null); ");
		tx.commit();
	}

	bool DatabaseWorker::documentExists(const Link& link)
	{
		pqxx::work txn{ c };

		pqxx::result r = txn.exec_prepared("document_count", static_cast<int>(link.protocol), link.hostName, link.query);

		txn.commit();

		if (!r.empty()) {
			int id = r[0][0].as<int>();

			if (id == 0)
			{
				return false;
			}
			else
			{
				return true;
			}
		}

		return false;
	}

	void DatabaseWorker::addDocumentWithWordsIfNotExists(const Link& link, const std::vector<std::string>& words)
	{
		std::lock_guard<std::mutex> lock(document_add_mutex);

		bool docExists = documentExists(link);
		if (docExists)
		{
			return;
		}

		pqxx::work doc_trx{ c };

		pqxx::result r = doc_trx.exec_prepared("insert_document", static_cast<int>(link.protocol), link.hostName, link.query);

		doc_trx.commit();

		if (!r.empty()) {
			int id = r[0][0].as<int>();

			pqxx::work words_trx{ c };

			std::map<std::string, int> wordCount = countWords(words);

			for (auto w : wordCount)
			{
				words_trx.exec_prepared("insert_word", w.first);
				words_trx.exec_prepared("insert_word_for_document", w.first, id, w.second);
			}

			words_trx.commit();
		}
	}
