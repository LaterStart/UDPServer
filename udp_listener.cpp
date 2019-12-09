#include <iostream>
#include <memory>
#include <boost/array.hpp>
#include "udp_listener.hpp"
#include "udp_socket.hpp"

using namespace std;
using namespace boost;
using namespace asio;
using namespace ip;

namespace udp_server {

    udp_listener::udp_listener() {
        context_ = new boost::asio::io_context();
        receive_event_signal.connect(boost::bind(&udp_listener::handle_receive_event, this, _1, _2, _3));
        confirm_event_signal.connect(boost::bind(&udp_listener::handle_confirm_event, this, _1, _2));
    }

    void udp_listener::listen_on(unsigned short port) {
        try
        {
            sockets_.push_back(new udp_socket(context_, port, this));
            sockets_.back()->listen();
        }
        catch (std::exception& ex)
        {
            cerr << ex.what() << endl;
        }
    }

    void udp_listener::handle_receive_event(udp_socket* udp_socket, size_t bytes_transferred, const boost::system::error_code& error) {

        log_receive_event(udp_socket, bytes_transferred, error);

        if (!error) {
            send_confirmation_message(udp_socket, bytes_transferred);
            if (data_reader_) data_reader_(udp_socket->data(bytes_transferred));
        }
    }

    void udp_listener::handle_confirm_event(udp_socket* udp_socket, const boost::system::error_code& error)
    {
        log_confirm_event(udp_socket, error);
    }

    void udp_listener::send_confirmation_message(udp_socket* udp_socket, size_t bytes_transferred) {
        string* message = new string("Successfully trasfered " + to_string(bytes_transferred) + " bytes");
        boost::shared_ptr<string> response(message);
        udp_socket->confirm(response);            
    }

    void udp_listener::log_receive_event(udp_socket* udp_socket, size_t bytes_transferred, const boost::system::error_code& error) {
        stringstream stream;
        if (!error) {
            auto time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
            stream << "\n" << std::ctime(&time) << "Received " << bytes_transferred << ((bytes_transferred != 1) ? " bytes" : " byte")
                << " from " << udp_socket->endpoint().address() << ":" << udp_socket->endpoint().port() << " on UDP: " << udp_socket->port << endl;
        }
        else stream << error.message() << endl;

        if (logger_) logger_(stream);
    }

    void udp_listener::log_confirm_event(udp_socket* udp_socket, const boost::system::error_code& error) {
        stringstream stream;
        if (!error) {
            stream << "Confirmed transfer from " << udp_socket->endpoint().address() << ":" << udp_socket->endpoint().port()
                << " on UDP: " << udp_socket->port << endl;
        }
        else stream << "Failed to confirm for " << udp_socket->endpoint().address() << ":" << udp_socket->endpoint().port()
            << " " << error.message() << " on UDP: " << udp_socket->port << endl;

        if (logger_) logger_(stream);
    }

    void udp_listener::run() {
        context_->run();
    }

    void udp_listener::restart() {
        context_->restart();
    }

    void udp_listener::stop_listening_on(unsigned short port) {
        auto it = find_if(sockets_.begin(), sockets_.end(), [&](udp_socket* socket) { return socket->port == port; });
        if (it != sockets_.end()) {
            (*it)->close();
            auto ptr = (*it);
            sockets_.erase(it);
            delete ptr;
        }
    }
}