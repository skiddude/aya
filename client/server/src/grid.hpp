#pragma once

#include <string>
#include <functional>
#include <thread>
#include <atomic>
#include <memory>
#include <boost/beast.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/strand.hpp>
#include <boost/smart_ptr/scoped_ptr.hpp>
#include <rapidjson/document.h>

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = net::ip::tcp;

class Grid : public std::enable_shared_from_this<Grid>
{
private:
    net::io_context ioc;
    tcp::acceptor acceptor;
    std::unique_ptr<std::thread> server_thread;
    std::atomic<bool> running;
    int port;

    void run_server();
    void do_accept();
    void handle_session(tcp::socket socket);
    std::string handle_request(const std::string& method, const std::string& path, const rapidjson::Document& body);
    bool is_valid_json(const std::string& str);

public:
    static boost::scoped_ptr<Grid> singleton;

    Grid(int port);
    ~Grid();

    bool start();
    void stop();
    bool is_running() const;

    std::function<std::string(const std::string& method, const std::string& path, const rapidjson::Document& json_body)> json_handler;
};

void start_grid_server(int port);
void stop_grid_server();
bool is_grid_server_running();