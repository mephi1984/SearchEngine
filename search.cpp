#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio.hpp>
#include <chrono>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <memory>
#include <string>

#include <pqxx/pqxx>

#include <sstream>
#include <iomanip>
#include <locale>
#include <codecvt>

#include <Windows.h>

#include <string>
#include <iostream>



#include <boost/locale.hpp>
/*
select protocol, host, query from words_documents join documents on words_documents.document_id = documents.id where word_id='OpenSSL' order by amount

*/

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
    return decode_unicode(url_decoded);
}


namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

class http_connection : public std::enable_shared_from_this<http_connection>
{
public:
    http_connection(tcp::socket socket, pqxx::connection& c)
        : socket_(std::move(socket))
        , conn(c)
    {
    }

    // Initiate the asynchronous operations associated with the connection.
    void
        start()
    {
        read_request();
        check_deadline();
    }

private:
    // The socket for the currently connected client.
    tcp::socket socket_;

    pqxx::connection& conn;

    // The buffer for performing reads.
    beast::flat_buffer buffer_{8192};

    // The request message.
    http::request<http::dynamic_body> request_;

    // The response message.
    http::response<http::dynamic_body> response_;

    // The timer for putting a deadline on connection processing.
    net::steady_timer deadline_{
        socket_.get_executor(), std::chrono::seconds(60)};

    // Asynchronously receive a complete request message.
    void
        read_request()
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
                    self->process_request();
            });
    }

    // Determine what needs to be done with the request message.
    void
        process_request()
    {
        response_.version(request_.version());
        response_.keep_alive(false);

        switch (request_.method())
        {
        case http::verb::get:
            response_.result(http::status::ok);
            response_.set(http::field::server, "Beast");
            create_response();
            break;
        case http::verb::post:
            response_.result(http::status::ok);
            response_.set(http::field::server, "Beast");
            create_response_post();
            break;

        default:
            // We return responses indicating an error if
            // we do not recognize the request method.
            response_.result(http::status::bad_request);
            response_.set(http::field::content_type, "text/plain");
            beast::ostream(response_.body())
                << "Invalid request-method '"
                << std::string(request_.method_string())
                << "'";
            break;
        }

        write_response();
    }

   
    void create_response()
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

    void create_response_post()
    {
        if (request_.target() == "/")
        {
            std::string s = buffers_to_string(request_.body().data());

            using namespace boost::locale;
            using namespace std;

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

            value = url_decode(value);

            std::cout << "value: " << value << std::endl;
            
            if (key != "search")
            {
                response_.result(http::status::not_found);
                response_.set(http::field::content_type, "text/plain");
                beast::ostream(response_.body()) << "File not found\r\n";
                return;
            }

            // Create system default locale
            generator gen;
            locale loc = gen("");
            locale::global(loc);

            auto word = to_lower(value);

            //std::cout << ">>>" << word << "<<<" << std::endl;

            pqxx::work txn{conn};

            pqxx::result r = txn.exec_prepared("search_pages", word);

            response_.set(http::field::content_type, "text/html");
            beast::ostream(response_.body())
                << "<html>\n"
                << "<head><meta charset=\"UTF-8\"><title>Search Engine</title></head>\n"
                << "<body>\n"
                << "<h1>Search Engine</h1>\n"
                << "<p>Response:<p>\n"
                << "<ul>\n";

            for (const auto& row : r) {

                std::string url = (row["protocol"].as<int>() == 0) ? "https://" : "http://";

                url += row["host"].as<std::string>();
                url += row["query"].as<std::string>();

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

    // Asynchronously transmit the response message.
    void
        write_response()
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
        check_deadline()
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
};

// "Loop" forever accepting new connections.
void
http_server(tcp::acceptor& acceptor, tcp::socket& socket, pqxx::connection& c)
{
    acceptor.async_accept(socket,
        [&](beast::error_code ec)
        {
            if (!ec)
                std::make_shared<http_connection>(std::move(socket), c)->start();
            http_server(acceptor, socket, c);
        });
}

int
main(int argc, char* argv[])
{
    SetConsoleCP(CP_UTF8);
    SetConsoleOutputCP(CP_UTF8);

    try
    {


        std::ifstream ini_file("C:\\Work\\Projects\\DiplomProject001\\settings.ini");

        if (!ini_file.is_open())
        {
            std::cout << "File settings.ini not found, quitting..." << std::endl;
            return -1;
        }

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
        if (my_map.count("db_host") == 0)
        {
            std::cout << "Please define db_host in the settings.ini file." << std::endl;
            return -1;
        }
        if (my_map.count("db_port") == 0)
        {
            std::cout << "Please define db_host in the settings.ini file." << std::endl;
            return -1;
        }
        if (my_map.count("db_name") == 0)
        {
            std::cout << "Please define db_name in the settings.ini file." << std::endl;
            return -1;
        }
        if (my_map.count("db_user") == 0)
        {
            std::cout << "Please define db_user in the settings.ini file." << std::endl;
            return -1;
        }
        if (my_map.count("db_password") == 0)
        {
            std::cout << "Please define db_password in the settings.ini file." << std::endl;
            return -1;
        }

        pqxx::connection c(
            "host=" + my_map["db_host"] +
            " port=" + my_map["db_port"] +
            " dbname=" + my_map["db_name"] +
            " user=" + my_map["db_user"] +
            " password=" + my_map["db_password"]);

        c.prepare("search_pages", "select protocol, host, query from words_documents join documents on words_documents.document_id = documents.id where word_id=$1 order by amount desc limit 10");


        auto const address = net::ip::make_address("0.0.0.0");
        unsigned short port = 8080;

        net::io_context ioc{1};

        tcp::acceptor acceptor{ioc, { address, port }};
        tcp::socket socket{ioc};
        http_server(acceptor, socket, c);

        ioc.run();
    }
    catch (std::exception const& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
}