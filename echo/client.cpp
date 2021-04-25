#include "client.h"

#include <iostream>

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

	}

	return 0;
}
