#include <iostream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <boost/beast/core.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl.hpp>
//#include <boost/json.hpp>
#include <openssl/ssl.h>
#include <string>
#include <vector>
#include <sstream>
#include <utility>
#include <regex>
#include <unordered_map>
#include <fstream>
#include <sstream>
#include <map>
#include <string>

#include <pqxx/pqxx>



namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
namespace ip = boost::asio::ip;
namespace ssl = boost::asio::ssl;
//namespace json = boost::json;
using tcp = boost::asio::ip::tcp;

enum class ProtocolType
{
	HTTP = 0,
	HTTPS = 1
};

struct Link
{
	ProtocolType protocol;
	std::string hostName;
	std::string query;
};

std::vector<std::string> explode(const std::string& s)
{
	std::vector<std::string> result;
	std::string delimiters(" \r\n\t.,;:!&()[]{}\"/=+-*'");

	size_t index = 0;
	std::string tempString;

	while (index < s.size())
	{
		if (delimiters.find(s[index]) == std::string::npos)
		{
			tempString += s[index];
		}
		else
		{
			if (tempString.size() > 0)
			{
				result.push_back(tempString);
			}
			tempString = "";
		}
		index++;
	}

	return result;
}

std::string removeHtmlTags(const std::string& html) {
	std::regex htmlTagPattern("<.*?>");
	return std::regex_replace(html, htmlTagPattern, "");
}


std::vector<std::string> extractLinks(const std::string& html) {
	std::vector<std::string> links;
	std::regex linkPattern("<a href=\"(.*?)\"");

	auto words_begin = std::sregex_iterator(html.begin(), html.end(), linkPattern);
	auto words_end = std::sregex_iterator();

	for (std::sregex_iterator i = words_begin; i != words_end; ++i) {
		std::smatch match = *i;
		std::string match_str = match.str();
		links.push_back(match_str.substr(9, match_str.size() - 10));
	}
	return links;
}

Link splitLink(const std::string& link)
{
	Link result;

	if (link.substr(0, 7) == "http://")
	{
		std::string withoutProtocol = link.substr(7);
		size_t slashPos = withoutProtocol.find('/');
		if (slashPos == std::string::npos)
		{
			throw std::runtime_error("invalid link");
		}
		std::string host = withoutProtocol.substr(0, slashPos);
		std::string query = withoutProtocol.substr(slashPos);
		result.hostName = host;
		result.query = query;
		result.protocol = ProtocolType::HTTP;
	}
	else if (link.substr(0, 8) == "https://")
	{
		std::string withoutProtocol = link.substr(8);
		size_t slashPos = withoutProtocol.find('/');
		if (slashPos == std::string::npos)
		{
			throw std::runtime_error("invalid link");
		}
		std::string host = withoutProtocol.substr(0, slashPos);
		std::string query = withoutProtocol.substr(slashPos);
		result.hostName = host;
		result.query = query;
		result.protocol = ProtocolType::HTTPS;
	}
	else
	{
		throw std::runtime_error("invalid link");
	}


	return result;
}

bool isText(const boost::beast::multi_buffer::const_buffers_type& b)
{
	for (auto itr = b.begin(); itr != b.end(); itr++)
	{
		for (int i = 0; i < (*itr).size(); i++)
		{
			if (*((const char*)(*itr).data() + i) == 0)
			{
				return false;
			}
		}
	}

	return true;
}

std::vector<Link> filterLinks(const std::vector<std::string>& rawLinks, ProtocolType protocol, const std::string& hostName)
{
	std::vector<Link> result;

	for (const auto& link : rawLinks)
	{
		if (link[0] == '/')
		{
			result.push_back({ ProtocolType::HTTPS, hostName, link });
		}
		else if ((link.substr(0, 7) == "http://") || (link.substr(0, 8) == "https://"))
		{
			result.push_back(splitLink(link));
		}
	}

	return result;
}


std::unordered_map<std::string, int> indexFile(const std::string& filename) {

	std::unordered_map<std::string, int> wordFrequencies;

	// Открываем файл
	std::ifstream file(filename);

	// Проверяем, удалось ли открыть файл
	if (!file.is_open()) {
		throw std::runtime_error("Could not open file");
	}

	// Строка для хранения текущей строки файла
	std::string line;

	// Читаем файл построчно
	while (std::getline(file, line)) {
		// Создаем поток строк для обработки строки
		std::istringstream iss(line);

		// Строка для хранения текущего слова
		std::string word;

		// Читаем слова из строки
		while (iss >> word) {
			// Увеличиваем частоту слова
			wordFrequencies[word]++;
		}
	}

	// Закрываем файл
	file.close();

	return wordFrequencies;
}

void createTables(pqxx::connection& c)
{
	pqxx::work tx{ c };

	tx.exec("create table if not exists words (id varchar(255) not null primary key); "); 
	tx.exec("create table if not exists documents (id serial primary key, protocol int not null, host text not null, query text not null); ");
	tx.exec("create table if not exists words_documents (id serial primary key, word_id varchar(255) not null references words(id), document_id integer not null references documents(id), amount integer not null); ");
	tx.commit();
}


void addDocumentWithWords(pqxx::connection& c, Link link, const std::vector<std::string>& words)
{
	pqxx::work txn{ c };

	pqxx::result r = txn.exec_prepared("insert_document", static_cast<int>(link.protocol), link.hostName, link.query);

	txn.commit();

	if (!r.empty()) {
		int id = r[0][0].as<int>();
		std::cout << "The id of the inserted row is: " << id << std::endl;

		pqxx::work txn2{ c };

		std::map<std::string, int> wordCount;

		for (auto& s : words)
		{
			if ((s.size() >= 3) && (s.size() < 255))
			{
				if (wordCount.count(s) == 0)
				{
					wordCount[s] = 1;
				}
				else
				{
					wordCount[s]++;
				}
			}
		}

		for (auto w : wordCount)
		{
			//TODO: TO LOWERCASE
			txn2.exec_prepared("insert_word", w.first);
			txn2.exec_prepared("insert_word_for_document", w.first, id, w.second);
		}

		txn2.commit();
	}

	
}


int main()
{
	try {

		pqxx::connection c(
			"host=localhost "
			"port=5432 "
			"dbname=lesson03 "
			"user=lesson03user "
			"password=lesson03user");


		c.prepare("insert_document", "INSERT INTO documents (protocol, host, query) VALUES ($1, $2, $3) RETURNING id;");
		c.prepare("insert_word", "INSERT INTO words (id) VALUES ($1) ON CONFLICT (id) DO NOTHING;");
		c.prepare("insert_word_for_document", "INSERT INTO words_documents (word_id, document_id, amount) VALUES ($1, $2, $3) RETURNING id;");


		std::ifstream ini_file("C:\\Work\\Projects\\DiplomProject001\\settings.ini");

		std::map<std::string, std::string> my_map;

		std::string line;
		while (std::getline(ini_file, line)) {
			size_t pos = line.find('=');
			if (pos == std::string::npos)
				continue;

			std::string key = line.substr(0, pos);
			std::string value = line.substr(pos + 1);
			my_map[key] = value;
		}

		if (my_map.count("start_page") == 0)
		{
			std::cout << "Please define start_page in the settings.ini file." << std::endl;
			return -1;
		}
		if (my_map.count("level") == 0)
		{
			std::cout << "Please define level in the settings.ini file." << std::endl;
			return -1;
		}

		std::string startPage = my_map["start_page"];
		int level = std::stoi(my_map["level"]);

		if (level <= 0)
		{
			std::cout << "Level must be 1 or greater." << std::endl;
			return -1;
		}

		Link link = splitLink(startPage);

		std::string host = link.hostName;
		//std::string query = "/images/9/97/HTAB.png";
		std::string query = link.query;

		// Initialize IO context
		net::io_context ioc;
		ssl::context ctx(ssl::context::tlsv13_client);
		ctx.set_default_verify_paths();
		// Set up an SSL context
		beast::ssl_stream<beast::tcp_stream> stream(ioc, ctx);
		stream.set_verify_mode(ssl::verify_none);
		stream.set_verify_callback([](bool preverified, ssl::verify_context& ctx) {
			return true; // Accept any certificate
			});
		// Enable SNI
		if (!SSL_set_tlsext_host_name(stream.native_handle(), host.c_str())) {
			beast::error_code ec{static_cast<int>(::ERR_get_error()), net::error::get_ssl_category()};
			throw beast::system_error{ec};
		}
		// Connect to the HTTPS server
		ip::tcp::resolver resolver(ioc);
		get_lowest_layer(stream).connect(resolver.resolve({ host, "https" }));
		get_lowest_layer(stream).expires_after(std::chrono::seconds(30));
		// Construct request
		http::request<http::empty_body> req{http::verb::get, query, 11};
		req.set(http::field::host, host);
		req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
		// Send the request
		stream.handshake(ssl::stream_base::client);
		http::write(stream, req);
		// Receive the response
		beast::flat_buffer buffer;
		http::response<http::dynamic_body> res;
		http::read(stream, buffer, res);

		if (!isText(res.body().data()))
		{
			std::cout << "This is not a text link, bailing out..." << std::endl;
			return 0;
		}

		std::string s = buffers_to_string(res.body().data());

		std::cout << s << std::endl;
		std::cout << "--------------" << std::endl;
		std::cout << s.substr(0, 10) << std::endl;
		std::cout << "-----------------" << std::endl;
		std::vector<std::string> rawLinks = extractLinks(s);

		std::vector<Link> links = filterLinks(rawLinks, link.protocol, link.hostName);

		for (auto& link : links)
		{
			std::cout << "link: " << link.hostName << " " << link.query << std::endl;
		}

		s = removeHtmlTags(s);

		std::vector<std::string> str = explode(s);

		std::cout << "-----------------" << std::endl;

		for (auto word : str)
		{
			if (word.length() >= 3)
			{
				std::cout << word << std::endl;
			}
		}

		std::cout << "--------------" << std::endl;

		addDocumentWithWords(c, link, str);



		//inFile.close();

		/*
		// Parse the JSON response
		json::error_code err;
		json::value j = json::parse(buffers_to_string(res.body().data()), err);
		std::cout << "IP address: " << j.at("ip").as_string() << std::endl;
		if (err) {
			std::cerr << "Error parsing JSON: " << err.message() << std::endl;
		}*/

		//std::cout << "IP address: " << j.at("ip").as_string() << std::endl;
		// Cleanup
		beast::error_code ec;
		stream.shutdown(ec);
		if (ec == net::error::eof) {
			ec = {};
		}
		if (ec) {
			throw beast::system_error{ec};
		}
	}
	catch (const std::exception& e)
	{
		std::cout << e.what() << std::endl;
	}
	return 0;
}