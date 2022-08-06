#pragma once

#include <string>

#include <boost/asio.hpp>
#include <boost/asio/ip/address.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>

namespace clog::utils {

class HTTPClient {
    using namespace boost;
    using Req = http::request<http::dynamic_body>;
    using Res = http::response<http::dynamic_body>;

    net::io_context m_ioc;
    tcp::resolver m_resolver;
    beast::tcp_stream m_stream;

  public:
    struct Response {
        std::string location;
        std::string body;
    };

    HTTPClient() : m_resolver{m_ioc}, m_stream{m_ioc} {}

    std::string send(http::verb method, std::string host, std::string port,
                     const std::string &target, const std::string &body = "") {
        const auto ip = m_resolver.resolve(host, port);
        auto req = http::request<http::string_body>{method, target, 10};
        req.set(http::field::host, host);
        req.set(http::field::content_type, "Application/json")
            req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

        m_stream.connect(ip);
        http::write(m_stream, req);

        return readRes().location;
    }

    std::string get(std::string host, std::string port, const std::string &target,
                    const std::string &body = "") {
        const auto ip = m_resolver.resolve(host, port);
        auto req = http::request<http::string_body>{http::verb::get, target, 10};
        req.set(http::field::host, host);
        req.set(http::field::content_type, "Application/json")
            req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

        m_stream.connect(ip);
        http::write(m_stream, req);

        return readRes().location;
    }

  private:
    Response readRes() {
        beast::flat_buffer buff;
        http::response<http::string_body> res;
        http::read(m_stream, buff, res);
        std::cout << res << std::endl;
        return {res.base()["Location"].to_string(), res.body()};
    }
};
} // namespace clog::utils
