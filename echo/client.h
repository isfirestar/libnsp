#if !defined NSTEST_ECHO_CLIENT_SESSION_20210425
#define NSTEST_ECHO_CLIENT_SESSION_20210425

#include "application_network_framework.hpp"
#include "endpoint.h"

class session_client : public nsp::tcpip::nsp_application_client
{
	nsp::tcpip::endpoint target_;

private:
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

