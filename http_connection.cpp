#include "http_connection.h"

#include <sstream>
#include <iomanip>
#include <locale>
#include <codecvt>

#include "common.h"

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = boost::asio::ip::tcp;



std::string url_decode(const std::string& encoded) {
    std::string res;
    std::istringstream iss(encoded);
    char ch;

    while (iss.get(ch)) {
        if (ch == '%') {
            int hex;
            iss >> std::hex >> hex;
            res += static_cast<char>(hex);
        }
        else {
            res += ch;
        }
    }

    return res;
}

std::string decode_unicode(const std::string& str) {
    std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> conv;
    std::string res;

    size_t pos = 0;
    while ((pos = str.find("&#", pos)) != std::string::npos) {
        size_t semi_colon = str.find(';', pos + 2);
        if (semi_colon == std::string::npos)
            break;

        std::string unicode_num_str = str.substr(pos + 2, semi_colon - pos - 2);
        char32_t unicode_num = std::stoi(unicode_num_str);
        res += conv.to_bytes(unicode_num);
        pos = semi_colon + 1;
    }

    return res;
}

std::string convert_to_utf8(const std::string& str) {
    std::string url_decoded = url_decode(str);
    //return decode_unicode(url_decoded);
    return url_decoded;
}

HttpConnection::HttpConnection(tcp::socket socket, ServerDatabaseWorker& databaseWorker)
        : socket_(std::move(socket))
        , databaseWorker_(databaseWorker)
    {
    }


    void
        HttpConnection::start()
    {
        readRequest();
        checkDeadline();
    }


    void
        HttpConnection::readRequest()
    {
        auto self = shared_from_this();

        http::async_read(
            socket_,
            buffer_,
            request_,
            [self](beast::error_code ec,
                std::size_t bytes_transferred)
            {
                boost::ignore_unused(bytes_transferred);
                if (!ec)
                    self->processRequest();
            });
    }

    void
        HttpConnection::processRequest()
    {
        response_.version(request_.version());
        response_.keep_alive(false);

        switch (request_.method())
        {
        case http::verb::get:
            response_.result(http::status::ok);
            response_.set(http::field::server, "Beast");
            createResponseGet();
            break;
        case http::verb::post:
            response_.result(http::status::ok);
            response_.set(http::field::server, "Beast");
            createResponsePost();
            break;

        default:
            response_.result(http::status::bad_request);
            response_.set(http::field::content_type, "text/plain");
            beast::ostream(response_.body())
                << "Invalid request-method '"
                << std::string(request_.method_string())
                << "'";
            break;
        }

        writeResponse();
    }


    void HttpConnection::createResponseGet()
    {
        if (request_.target() == "/")
        {
            response_.set(http::field::content_type, "text/html");
            beast::ostream(response_.body())
                << "<html>\n"
                << "<head><meta charset=\"UTF-8\"><title>Search Engine</title></head>\n"
                << "<body>\n"
                << "<h1>Search Engine</h1>\n"
                << "<p>Welcome!<p>\n"
                << "<form action=\"/\" method=\"post\">\n"
                << "    <label for=\"search\">Search:</label><br>\n"
                << "    <input type=\"text\" id=\"search\" name=\"search\"><br>\n"
                << "    <input type=\"submit\" value=\"Search\">\n"
                << "</form>\n"
                << "</body>\n"
                << "</html>\n";
        }
        else
        {
            response_.result(http::status::not_found);
            response_.set(http::field::content_type, "text/plain");
            beast::ostream(response_.body()) << "File not found\r\n";
        }
    }

    void HttpConnection::createResponsePost()
    {
        if (request_.target() == "/")
        {
            std::string s = buffers_to_string(request_.body().data());

            std::cout << "s: " << s << std::endl;

            size_t pos = s.find('=');
            if (pos == std::string::npos)
            {
                response_.result(http::status::not_found);
                response_.set(http::field::content_type, "text/plain");
                beast::ostream(response_.body()) << "File not found\r\n";
                return;
            }

            std::string key = s.substr(0, pos);
            std::string value = s.substr(pos + 1);

            std::string utf8value = convert_to_utf8(value);
            std::cout << "key: " << key << std::endl;
            std::cout << "value: " << value << std::endl;
            std::cout << "utf8value: " << utf8value << std::endl;

            if (key != "search")
            {
                response_.result(http::status::not_found);
                response_.set(http::field::content_type, "text/plain");
                beast::ostream(response_.body()) << "File not found\r\n";
                return;
            }

            std::vector<std::string> searchResult = databaseWorker_.searchPages(utf8value);

            response_.set(http::field::content_type, "text/html");
            beast::ostream(response_.body())
                << "<html>\n"
                << "<head><meta charset=\"UTF-8\"><title>Search Engine</title></head>\n"
                << "<body>\n"
                << "<h1>Search Engine</h1>\n"
                << "<p>Response:<p>\n"
                << "<ul>\n";

            for (const auto& url : searchResult) {

                beast::ostream(response_.body())
                    << "<li><a href=\""
                    << url << "\">"
                    << url << "</a></li>";
            }

            beast::ostream(response_.body())
                << "</ul>\n"
                << "</body>\n"
                << "</html>\n";
        }
        else
        {
            response_.result(http::status::not_found);
            response_.set(http::field::content_type, "text/plain");
            beast::ostream(response_.body()) << "File not found\r\n";
        }
    }

    void
        HttpConnection::writeResponse()
    {
        auto self = shared_from_this();

        response_.content_length(response_.body().size());

        http::async_write(
            socket_,
            response_,
            [self](beast::error_code ec, std::size_t)
            {
                self->socket_.shutdown(tcp::socket::shutdown_send, ec);
                self->deadline_.cancel();
            });
    }

    // Check whether we have spent enough time on this connection.
    void
        HttpConnection::checkDeadline()
    {
        auto self = shared_from_this();

        deadline_.async_wait(
            [self](beast::error_code ec)
            {
                if (!ec)
                {
                    // Close socket to cancel any outstanding operation.
                    self->socket_.close(ec);
                }
            });
    }

