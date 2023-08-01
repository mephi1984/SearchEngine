#include "http_utils.h"

#include <regex>
#include <iostream>

#include <boost/beast/core.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl.hpp>
#include <openssl/ssl.h>

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
namespace ip = boost::asio::ip;
namespace ssl = boost::asio::ssl;
//namespace json = boost::json;
using tcp = boost::asio::ip::tcp;

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


std::unordered_set<Link> filterLinks(const std::vector<std::string>& rawLinks, ProtocolType protocol, const std::string& hostName)
{
	std::unordered_set<Link> result;

	for (const auto& link : rawLinks)
	{
		if (link[0] == '/')
		{
			result.insert({ ProtocolType::HTTPS, hostName, link });
		}
		else if ((link.substr(0, 7) == "http://") || (link.substr(0, 8) == "https://"))
		{
			result.insert(splitLink(link));
		}
	}

	return result;
}


std::string GetHtmlContent(const Link& link)
{

	std::string result;

	try
	{
		std::string host = link.hostName;
		std::string query = link.query;


		net::io_context ioc;
		ssl::context ctx(ssl::context::tlsv13_client);
		ctx.set_default_verify_paths();

		beast::ssl_stream<beast::tcp_stream> stream(ioc, ctx);
		stream.set_verify_mode(ssl::verify_none);

		stream.set_verify_callback([](bool preverified, ssl::verify_context& ctx) {
			return true; // Accept any certificate
			});


		if (!SSL_set_tlsext_host_name(stream.native_handle(), host.c_str())) {
			beast::error_code ec{static_cast<int>(::ERR_get_error()), net::error::get_ssl_category()};
			throw beast::system_error{ec};
		}

		ip::tcp::resolver resolver(ioc);
		get_lowest_layer(stream).connect(resolver.resolve({ host, "https" }));
		get_lowest_layer(stream).expires_after(std::chrono::seconds(30));


		http::request<http::empty_body> req{http::verb::get, query, 11};
		req.set(http::field::host, host);
		req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

		stream.handshake(ssl::stream_base::client);
		http::write(stream, req);

		beast::flat_buffer buffer;
		http::response<http::dynamic_body> res;
		http::read(stream, buffer, res);

		if (isText(res.body().data()))
		{
			result = buffers_to_string(res.body().data());
		}
		else
		{
			std::cout << "This is not a text link, bailing out..." << std::endl;
		}

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

	return result;
}