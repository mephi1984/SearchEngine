#include "server_database_worker.h"
#include "http_utils.h"
#include "common.h"


ServerDatabaseWorker::ServerDatabaseWorker(const IniParser& iniParser)
		: DatabaseWorker(iniParser)
	{

	c.prepare("search_pages_1word", "select protocol, host, query, amount from documents join(select * from words_documents where word_id = $1) as subquery on subquery.document_id = documents.id order by amount desc limit 10");
	c.prepare("search_pages_2words", "select protocol, host, query, sum(amount) from documents join(select * from words_documents where word_id = $1 or word_id = $2) as subquery on subquery.document_id = documents.id group by(protocol, host, query) order by sum desc limit 10");
	c.prepare("search_pages_3words", "select protocol, host, query, sum(amount) from documents join(select * from words_documents where word_id = $1 or word_id = $2 or word_id = $3) as subquery on subquery.document_id = documents.id group by(protocol, host, query) order by sum desc limit 10");
	c.prepare("search_pages_4words", "select protocol, host, query, sum(amount) from documents join(select * from words_documents where word_id = $1 or word_id = $2 or word_id = $3  or word_id = $4) as subquery on subquery.document_id = documents.id group by(protocol, host, query) order by sum desc limit 10");

	}

std::vector<std::string> ServerDatabaseWorker::searchPages(const std::string& query)
{
	std::vector<std::string> result;

	auto queryLowerCase = wordToLowerCase(query);

	std::cout << "lower case: " << queryLowerCase;

	std::vector<std::string> words = explode(queryLowerCase);

	pqxx::work txn{c};

	pqxx::result r;

	if (words.size() == 0)
	{
		return result;
	}
	else if (words.size() == 1)
	{
		r = txn.exec_prepared("search_pages_1word", words[0]);
	}
	else if (words.size() == 2)
	{
		r = txn.exec_prepared("search_pages_2words", words[0], words[1]);
	}
	else if (words.size() == 3)
	{
		r = txn.exec_prepared("search_pages_3words", words[0], words[1], words[2]);
	}
	else if (words.size() >= 4)
	{
		r = txn.exec_prepared("search_pages_4words", words[0], words[1], words[2], words[3]);
	}


	for (const auto& row : r) {

		std::string url = (row["protocol"].as<int>() == 0) ? "https://" : "http://";

		url += row["host"].as<std::string>();
		url += row["query"].as<std::string>();
		result.push_back(url);
	}

	return result;

}