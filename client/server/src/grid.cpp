#include "grid.hpp"
#include <boost/beast.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/strand.hpp>
#include <boost/config.hpp>
#include <iostream>
#include <memory>
#include <string>
#include <thread>

boost::scoped_ptr<Grid> Grid::singleton;

Grid::Grid(int port)
    : acceptor(ioc)
    , running(false)
    , port(port)
{
}

Grid::~Grid()
{
    stop();
}

bool Grid::start()
{
    if (running.load())
    {
        return false;
    }

    try
    {
        auto const address = net::ip::make_address("0.0.0.0");
        tcp::endpoint endpoint{address, static_cast<unsigned short>(port)};

        acceptor.open(endpoint.protocol());
        acceptor.set_option(net::socket_base::reuse_address(true));
        acceptor.bind(endpoint);
        acceptor.listen(net::socket_base::max_listen_connections);

        running.store(true);
        server_thread = std::make_unique<std::thread>(&Grid::run_server, this);

        std::cout << "Grid started on port " << port << std::endl;
        return true;
    }
    catch (std::exception const& e)
    {
        std::cerr << "Error starting Grid: " << e.what() << std::endl;
        return false;
    }
}

void Grid::stop()
{
    if (!running.load())
    {
        return;
    }

    running.store(false);

    try
    {
        acceptor.close();
        ioc.stop();
    }
    catch (std::exception const& e)
    {
        std::cerr << "Error stopping Grid: " << e.what() << std::endl;
    }

    if (server_thread && server_thread->joinable())
    {
        server_thread->join();
    }

    std::cout << "Grid stopped" << std::endl;
}

bool Grid::is_running() const
{
    return running.load();
}

void Grid::run_server()
{
    do_accept();
    ioc.run();
}

void Grid::do_accept()
{
    
}

void Grid::handle_session(tcp::socket socket)
{
    try
    {
        beast::flat_buffer buffer;
        http::request<http::string_body> req;

        // Read the request
        http::read(socket, buffer, req);

        http::response<http::string_body> res;
        res.version(req.version());
        res.set(http::field::server, "Grid/1.0");
        res.set(http::field::content_type, "application/json");
        res.set(http::field::access_control_allow_origin, "*");
        res.set(http::field::access_control_allow_methods, "GET, POST");
        res.set(http::field::access_control_allow_headers, "Content-Type");

        std::string method = std::string(req.method_string());
        std::string path = std::string(req.target());
        std::string body = req.body();
/*
        if (method == "GET" || method == "POST")
        {
            if (method == "POST" && !body.empty())
            {
                rapidjson::Document doc;
                if (doc.Parse(body.c_str()).HasParseError())
                {
                    res.result(http::status::bad_request);
                    res.body() = R"({"error":"Invalid JSON"})";
                }
                else
                {
                    std::string response_body = handle_request(method, path, doc);
                    res.result(http::status::ok);
                    res.body() = response_body;
                }
            }
            else
            {
                std::string response_body = handle_request(method, path, rapidjson::Document());
                res.result(http::status::ok);
                res.body() = response_body;
            }
        }
        else
        {
            res.result(http::status::method_not_allowed);
            res.body() = R"({"error":"Method not allowed"})";
        }
*/
        res.prepare_payload();

        http::write(socket, res);
        socket.shutdown(tcp::socket::shutdown_send);
    }
    catch (std::exception const& e)
    {
        std::cerr << "Session error: " << e.what() << std::endl;
    }
}

std::string Grid::handle_request(const std::string& method, const std::string& path, const rapidjson::Document& json_body)
{
    if (json_handler)
    {
        return json_handler(method, path, json_body);
    }

    return R"({"status":"ok","message":"Grid server is running"})";
}

bool Grid::is_valid_json(const std::string& str)
{
    if (str.empty())
        return false;

    rapidjson::Document doc;
    if (doc.Parse(str.c_str()).HasParseError())
    {
        return false;
    }

    return doc.IsObject() || doc.IsArray();
}

void start_grid_server(int port)
{
    if (!Grid::singleton)
    {
        Grid::singleton.reset(new Grid(port));
    }
    Grid::singleton->start();
}

void stop_grid_server()
{
    if (Grid::singleton)
    {
        Grid::singleton->stop();
    }
}

bool is_grid_server_running()
{
    return Grid::singleton && Grid::singleton->is_running();
}