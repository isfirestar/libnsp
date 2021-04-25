#if !defined NSHOST_ECHO_ARGS_H
#define NSHOST_ECHO_ARGS_H

#include "singleton.hpp"
#include "endpoint.h"

#define SESS_TYPE_UNKNOWN       (-1)
#define SESS_TYPE_SERVER         ('s')
#define SESS_TYPE_CLIENT         ('c')

#define NSTEST_MODEL_ECHO			(1)
#define NSTEST_MODEL_DATA_TRANSFER	(2)

// parameters while running data transfer model
struct data_transfer_context
{
	int interval_ = 100;
	int threads_ = 1;
	int length_ = 1024;
};

class foundation_argument
{
	friend class nsp::toolkit::singleton<foundation_argument>;

	int type_ = SESS_TYPE_SERVER;
    char host_[128];
    uint16_t port_ = 10256;
    int echo_ = 0;
    int mute_ = 0;
    data_transfer_context dtctx_;

    // this class MUST invoke in singleton model. construct any other objects are NOT permissive.
	foundation_argument();
	void display_usage();
	void display_author_information();

public:
	~foundation_argument();
	int load(int argc, char **argv);
	// application startup as client or server
	int get_type() const;
	// application startup as a 'ECHO' model or 'data-transfer' model
	int get_model() const;
	// get display model in server
	int get_mute() const;
	// this interface use to obtain the endpoint address.
	// indicate the service listen address associated a server, and indicate the target server host address associated a client
	int get_initial_endpoint(nsp::tcpip::endpoint &initep) const;
	// get context of parameters when using data-transfer model
	const data_transfer_context &get_data_transfer_context(data_transfer_context &context);
};

typedef nsp::toolkit::singleton<foundation_argument> application_initial_argument;

#endif
