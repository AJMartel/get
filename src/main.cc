/**
 * Copyright (C) 2015 Kurt Kanzenbach <kurt@kmk-computers.de>
 *
 * Just a simple tool for fetching a file by URL.
 *
 */

#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <libgen.h>

#include <unistd.h>

#include "config.h"
#include "protocol_dispatcher.h"
#include "logger.h"

static inline
void print_usage_and_die()
{
    std::cerr << "usage: get [options] <url>" << std::endl;
    std::cerr << "  options:" << std::endl;
    std::cerr << "    -p       : show progressbar if available" << std::endl;
    std::cerr << "    -u <user>: username" << std::endl;
    std::cerr << "    -k <pw>  : password" << std::endl;
    std::cerr << "    -f       : do not follow HTTP redirects" << std::endl;
    std::cerr << "    -v       : verify server's SSL certificate" << std::endl;
    std::cerr << "    -2       : use SSL version 2" << std::endl;
    std::cerr << "    -3       : use SSL version 3" << std::endl;
    std::cerr << "get version 1.2 (C) Kurt Kanzenbach <kurt@kmk-computers.de>" << std::endl;
    exit(-1);
}

int main(int argc, char *argv[])
{
    // parse args
    Config *config = Config::instance();
    std::string url;
    std::string user;
    std::string pw;
    bool show_pg = false;
    bool dont_follow_redirects = false;
    bool verify_peer = false;
    bool use_sslv2 = false;
    bool use_sslv3 = false;
    int c;

    if (argc <= 1)
        print_usage_and_die();

    while ((c = getopt(argc, argv, "23vpfu:k:")) != -1) {
        switch (c) {
        case 'p':
            show_pg = 1;
            break;
        case 'f':
            dont_follow_redirects = 1;
            break;
        case 'u':
            user = optarg;
            break;
        case 'k':
            pw = optarg;
            break;
        case 'v':
            verify_peer = true;
            break;
        case '2':
            use_sslv2 = true;
            break;
        case '3':
            use_sslv3 = true;
            break;
        case '?':
            print_usage_and_die();
        case ':':
            print_usage_and_die();
        }
    }
    if (optind >= argc)
        print_usage_and_die();
    url = argv[optind];

    config->show_pg() = show_pg;
    config->follow_redirects() = !dont_follow_redirects;
    config->verify_peer() = verify_peer;
    config->use_sslv2() = use_sslv2;
    config->use_sslv3() = use_sslv3;

    // dispatch
    try {
        ProtocolDispatcher dispatcher(url, user, pw);
        dispatcher.dispatch();
    } catch (const std::exception&) {
        log_info("Unfortunately an error has occured :(. For more information read "
                 "error messages above.");
        std::exit(-1);
    }

    return 0;
}