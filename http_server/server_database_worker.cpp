#include "server_database_worker.h"
#include "../common/common_functions.h"


ServerDatabaseWorker::ServerDatabaseWorker(const IniParser& iniParser)
	: DatabaseWorker(iniParser)
{

	c.prepare("search_pages_1word",
		"select d.protocol, d.host, d.query, sum(wd1.amount) as totalsum "
		"FROM documents d "
		"JOIN words_documents wd1 ON d.id = wd1.document_id "
		"JOIN words w1 ON wd1.word_id = w1.id AND w1.id = $1 "
		"group by(d.protocol, d.host, d.query) "
		"order by totalsum desc limit 10"
	);

	c.prepare("search_pages_2words", "select d.protocol, d.host, d.query, sum(wd1.amount) + sum(wd2.amount) as totalsum "
		"FROM documents d "
		"JOIN words_documents wd1 ON d.id = wd1.document_id "
		"JOIN words w1 ON wd1.word_id = w1.id AND w1.id = $1 "

		"JOIN words_documents wd2 ON d.id = wd2.document_id "
		"JOIN words w2 ON wd2.word_id = w2.id AND w2.id = $2 "
		"group by(d.protocol, d.host, d.query) "
		"order by totalsum desc limit 10");

	c.prepare("search_pages_3words", "select d.protocol, d.host, d.query, sum(wd1.amount) + sum(wd2.amount) + sum(wd3.amount) as totalsum "
		"FROM documents d "
		"JOIN words_documents wd1 ON d.id = wd1.document_id "
		"JOIN words w1 ON wd1.word_id = w1.id AND w1.id = $1 "

		"JOIN words_documents wd2 ON d.id = wd2.document_id "
		"JOIN words w2 ON wd2.word_id = w2.id AND w2.id = $2 "

		"JOIN words_documents wd3 ON d.id = wd3.document_id "
		"JOIN words w3 ON wd3.word_id = w3.id AND w3.id = $3 "

		"group by(d.protocol, d.host, d.query) "
		"order by totalsum desc limit 10");

	c.prepare("search_pages_4words", "select d.protocol, d.host, d.query, sum(wd1.amount) + sum(wd2.amount) + sum(wd3.amount) + sum(wd4.amount) as totalsum "
		"FROM documents d "
		"JOIN words_documents wd1 ON d.id = wd1.document_id "
		"JOIN words w1 ON wd1.word_id = w1.id AND w1.id = $1 "

		"JOIN words_documents wd2 ON d.id = wd2.document_id "
		"JOIN words w2 ON wd2.word_id = w2.id AND w2.id = $2 "

		"JOIN words_documents wd3 ON d.id = wd3.document_id "
		"JOIN words w3 ON wd3.word_id = w3.id AND w3.id = $3 "

		"JOIN words_documents wd4 ON d.id = wd4.document_id "
		"JOIN words w4 ON wd4.word_id = w4.id AND w4.id = $4 "

		"group by(d.protocol, d.host, d.query) "
		"order by totalsum desc limit 10");

}

std::vector<std::string> ServerDatabaseWorker::searchPages(const std::string& query)
{
	std::vector<std::string> result;

	auto queryLowerCase = wordToLowerCase(query);

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