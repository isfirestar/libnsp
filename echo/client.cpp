#include "client.h"

#include <iostream>
#include <sstream>
#include <chrono>
#include <iomanip>

#include "log.h"
#include "os_util.hpp"

#include "args.h"

session_client::session_client() : nsp::tcpip::nsp_application_client()
{
	;
}

session_client::session_client( HTCPLINK lnk) : nsp::tcpip::nsp_application_client(lnk)
{
	;
}

session_client::~session_client()
{
	;
}

void session_client::on_disconnected( const HTCPLINK previous )
{
	loinfo("client") << "on_disconnected";
}

void session_client::on_recvdata( const std::basic_string<unsigned char> &pkt )
{
	// packet::head testpkt;
	// int n = pkt.size();
	// testpkt.build(pkt.data(), n);
	// loinfo("client") << "recv packet id:" << testpkt.id << std::endl;
	if ( NSTEST_MODEL_ECHO == application_initial_argument::instance()->get_model() ) {
		this->on_echo_display(pkt.data(), pkt.size());
	} else {
		this->rx_total_ += pkt.size();
	}
}

void session_client::on_connected()
{
	;
}

int session_client::on_echo_display(const unsigned char *data, int size)
{
	if (!data || size <= 0) {
		std::string input;

		std::cout << "input:$ ";
		std::cin >> input;
		return this->send( reinterpret_cast<const unsigned char *>(input.c_str()), input.size() );
	} else {
		std::string output;

		output = "[income ";
		output += this->remote_.ipv4();
		output += ":";
		output += std::to_string(this->remote_.port());
		output += "] ";
		output += reinterpret_cast<const char *>(data);

		std::cout << output << std::endl;
		return this->on_echo_display(NULL, 0);
	}
}

void session_client::on_data_block_transfer()
{
	data_transfer_context context;
	application_initial_argument::instance()->get_data_transfer_context(context);

	const unsigned char *p = new (std::nothrow) unsigned char[context.length_];
	assert(p);
	while (!this->data_transfer_exit_) {
		if ( this->send( p,  context.length_ ) <= 0 ) {
			this->close();
			break;
		}

		this->tx_total_ += context.length_;
		std::this_thread::sleep_for(std::chrono::milliseconds(context.interval_));
	}

	delete []p;
}

const std::string session_client::convert_to_display(double bps)
{
        std::stringstream ss;

        if ( is_float_larger_than(bps, 1024.0) ) {
                bps /= 1024;
                if ( is_float_larger_than(bps, 1024.0) ) {
                        bps /= 1024;
                        ss << std::fixed << std::setprecision(2) << bps << "Mbps";
                        return ss.str();
                } else {
                        ss << std::fixed << std::setprecision(2) << bps << "Kbps";
                        return ss.str();
                }
        } else {
                ss << std::fixed << std::setprecision(2) << bps << "bps";
                return ss.str();
        }
}

void session_client::on_transfer_statistic_interruption()
{
	std::string input;

	while (!this->data_transfer_exit_) {
		auto begin_tick = std::chrono::high_resolution_clock::now();
		std::cout << "input:$ ";
		std::cin >> input;
		auto end_tick = std::chrono::high_resolution_clock::now();
		auto elapsed_tick= std::chrono::duration_cast<std::chrono::milliseconds>(end_tick - begin_tick);

		auto rxbps = (double)this->rx_total_.exchange(0) / elapsed_tick.count() * 8000;
		auto txbps = (double)this->tx_total_.exchange(0) / elapsed_tick.count() * 8000;

		if (input == "statistic" || input == "x") {
			std::cout << "Rx: " << convert_to_display(rxbps) << " Tx:" << convert_to_display(txbps) << std::endl;
		}

		if (input == "rx" || input == "r") {
			std::cout << "Rx: " << convert_to_display(rxbps) << std::endl;
		}

		if (input == "tx" || input == "t") {
			std::cout << "Tx: " << convert_to_display(txbps) << std::endl;
		}
	}
}

int session_client::begin()
{
	if ( application_initial_argument::instance()->get_initial_endpoint(this->target_) < 0 ) {
		loerror("client") << "fails obtain target host endpoint.";
		return -1;
	}

	if ( this->create() < 0 ) {
		loerror("client") << "fails on create session TCP link.";
		return -1;
	}

	if ( this->connect(this->target_) < 0 ) {
		loerror("client") << "fails on connect to target server host.";
		return -1;
	}

	if ( NSTEST_MODEL_ECHO == application_initial_argument::instance()->get_model() ) {
		this->on_echo_display(NULL, 0);
		// hang the process and waitting for user input
		nsp::os::pshang();
	} else {
		data_transfer_context context;
		application_initial_argument::instance()->get_data_transfer_context(context);
		for (int i = 0; i < context.threads_; i++) {
			std::thread t(std::bind(&session_client::on_data_block_transfer, this));
			this->data_transfer_threads_.push_back(std::move(t));
		}
		// display the statistic information when interrupt by user input command
		this->on_transfer_statistic_interruption();
	}

	return 0;
}
