#include "args.h"

#include <cstring>
#include <getopt.h>

enum ope_index {
    kOptIndex_GetHelp = 'h',
    kOptIndex_GetVersion = 'v',

    kOptIndex_SetPort = 'P',
    kOptIndex_EchoModel = 'e',

    kOptIndex_Server = 's',
    kOptIndex_MuteServer = 'm',

    kOptIndex_Client = 'c',
    kOptIndex_MultiThreadingCount = 't',
    kOptIndex_Interval = 'i',
    kOptIndex_DataLength = 'l',
};

static const struct option long_options[] = {
    {"help", no_argument, NULL, kOptIndex_GetHelp},
    {"version", no_argument, NULL, kOptIndex_GetVersion},
    {"port", required_argument, NULL, kOptIndex_SetPort},
    {"echo", no_argument, NULL, kOptIndex_EchoModel},
    {"server", no_argument, NULL, kOptIndex_Server},
    {"mute", no_argument, NULL, kOptIndex_MuteServer},
    {"client", required_argument, NULL, kOptIndex_Client},
    {"multi-threading", required_argument, NULL, kOptIndex_MultiThreadingCount},
    {"interval", required_argument, NULL, kOptIndex_Interval},
    {"data-length", required_argument, NULL, kOptIndex_DataLength},
    {NULL, 0, NULL, 0}
};

// foundation argument implement
foundation_argument::foundation_argument()
{
    strcpy(host_, "0.0.0.0");
}

foundation_argument::~foundation_argument()
{
    ;
}

int foundation_argument::load(int argc, char **argv)
{
    int opt_index;
    int opt;
    int retval = 0;
    char shortopts[128];

    /* double '::' meat option may have argument or not,
        one ':' meat option MUST have argument,
        no ':' meat option MUST NOT have argument */
    strcpy(shortopts, "hvP:esmc:t:i:l:");
    opt = getopt_long(argc, argv, shortopts, long_options, &opt_index);
    while (opt != -1) {
        switch (opt) {
            case 'h':
                display_usage();
                return -1;
            case 'v':
                display_author_information();
                return -1;
            case 'P':
                assert(optarg);
                this->port_ = (uint16_t) strtoul(optarg, NULL, 10);
                break;
            case 'e':
                this->echo_ = 1;
                break;
            case 's':
                this->type_ = opt;
                break;
            case 'm':
                this->mute_ = 1;
                break;
            case 'c':
                assert (optarg);
                strcpy(this->host_, optarg);
                this->type_ = opt;
                break;
            case 't':
                assert(optarg);
                this->dtctx_.threads_ = atoi(optarg);
                break;
            case 'i':
                assert(optarg);
                this->dtctx_.interval_ = atoi(optarg);
                break;
            case 'l':
                assert(optarg);
                this->dtctx_.length_ = atoi(optarg);
                break;
            case '?':
                printf("?\n");
            case 0:
                printf("0\n");
            default:
                display_usage();
                return -1;
        }
        opt = getopt_long(argc, argv, shortopts, long_options, &opt_index);
    }

    if ( this->type_ == SESS_TYPE_CLIENT && 0 == strcasecmp( this->host_, "0.0.0.0" ) ) {
        display_usage();
        return -1;
    }

    return retval;
}

void foundation_argument::display_usage()
{
    static const char *usage_context =
            "usage: nstest {-v|--version|-h|--help}\n"
            "\nbelow options are effective both server and client:\n"
            "[-P | --port [port]]\tto specify the connect target in client or change the local listen port in server\n"
            "[-e | --echo]\trun program in echo model\n"
            "\nbelow options are only effective in server:\n"
            "[-m | --mute]\tonly effective in server\n"
            "\t\twhen this argument has been specified, server are in silent model, nothing response to client\n"
            "\t\totherwise in default, all packages will completely consistent response to client\n"
            "[-s | --server]\trun program as a server\n"
            "\nbelow options are only effective in client:\n"
            "[-c | --client [target]]\trun program as a client, and target host must specified\n"
            "[-t | --multi-threading [count]]\tspecify count of threads to send data to server\n"
            "[-i | --interval [milliseconds]]\tspecifies the interval at which the sending thread sends data to server\n"
            "[-l | --data-length [bytes]]\tspecify the size of each packets to send to server\n"
            ;

    printf("%s", usage_context);
}

#define VERSION_STRING "VersionOfProgramDefinition-ChangeInMakefile"
void foundation_argument::display_author_information()
{
    static char author_context[512];
    sprintf(author_context, "nstest\n%s\n"
            "Copyright (C) 2017 Neo.Anderson\n"
            "For bug reporting instructions, please see:\n"
            "<http://www.nsplibrary.com.cn/>.\n"
            "For help, type \"help\".\n", VERSION_STRING);
    printf("%s", author_context);
}

int foundation_argument::get_type() const
{
    return this->type_;
}

int foundation_argument::get_model() const
{
    return (0 == this->echo_) ? NSTEST_MODEL_DATA_TRANSFER : NSTEST_MODEL_ECHO;
}

int foundation_argument::get_mute() const
{
    return this->mute_;
}

int foundation_argument::get_initial_endpoint(nsp::tcpip::endpoint &initep) const
{
    return nsp::tcpip::endpoint::build(this->host_, this->port_, initep);
}

const data_transfer_context &foundation_argument::get_data_transfer_context(data_transfer_context &context)
{
    context = this->dtctx_;
    return context;
}
