#if !defined NSTEST_ECHO_CLIENT_SESSION_20210425
#define NSTEST_ECHO_CLIENT_SESSION_20210425

#include "application_network_framework.hpp"
#include "endpoint.h"

#include <thread>
#include <vector>

class session_client : public nsp::tcpip::nsp_application_client
{
	nsp::tcpip::endpoint target_;
	std::vector<std::thread>	data_transfer_threads_;
	int data_transfer_exit_ = 0;
	std::atomic<int64_t> rx_total_ = { 0 };
	std::atomic<int64_t> tx_total_ = { 0 };

private:
	const std::string convert_to_display(double bps);
	void on_data_block_transfer();
	void on_transfer_statistic_interruption();
	int on_echo_display(const unsigned char *data, int size);

public:
	session_client();
	// this construction function MUST support if the session deem as the server template parameter
	session_client( HTCPLINK lnk);
	~session_client();
	virtual void on_disconnected( const HTCPLINK previous ) override final;
	virtual void on_recvdata( const std::basic_string<unsigned char> &pkt ) override final;
	virtual void on_connected() override final;

public:
	int begin();
};

#endif

