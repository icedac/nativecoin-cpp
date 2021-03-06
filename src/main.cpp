/****************************************************************************
 *
 *  main.cpp
 *      ($\nativecoin-cpp\src)
 *
 *  by icedac 
 *
 ***/
#include "stdafx.h"

#include "nc.h"
#include "blockchain.h"
#include "node.h"

#include "util.h"
#include <iomanip>
#include <time.h>

#define ASIO_STANDALONE
#include <asio.hpp>

#pragma comment( lib, "ws2_32.lib" )

#pragma message( "          -----------------------" )
#pragma message( "          [ compiler: " __COMPILER_NAME )
#ifdef USE_CPP_17
#pragma message( "          [ c++std: C++17      ]" )
#else
#pragma message( "          [ c++std: C++14      ]" )
#endif
#pragma message( "          -----------------------" )

void test_interface() {
    nc::node n;
    n.init();

    std::cout << n.bc.debug_as_string();

    const auto& genesis = n.get_wallet("genesis");
    const auto& w1 = n.create_wallet("icedac@gmail.com");


    nc::coin amount(10);
    amount += nc::coin::from_satoshi(50000);

    n.create_transaction(genesis, w1.get_address(), amount);

    // pool
    for (const auto& t : n.pool) {
        std::cout << t.as_debug_string() << "\n";
    }

    n.commit_block();

    std::cout << n.bc.debug_as_string();

    std::cout << n.as_debug_string(genesis) << std::endl;
    std::cout << n.as_debug_string(w1) << std::endl;
}

int main()
{
    if (0)
    {
        // auto now = nc::util::time_since_epoch();
        using std::chrono::system_clock;
        auto now = system_clock::to_time_t(system_clock::now());
        tm _tm;
        ::localtime_s(&_tm, &now);
        std::cout << std::put_time(&_tm, "%F %T") << '\n';
        std::cout << "ts: " << now << "\n";

        auto hash_str = nc::crypt::sha256("123124124121255");
        std::cout << "hash_str: " << nc::crypt::sha256("123456").get() << "\n";

        auto bb = nc::block{ 1, nc::sha256_hash::from_string("1234"), 123,{}, 1, 10 };
        nc::coin a{ 10 }, b{ 20 };
        auto c = a + b;
        // c.satoshi;
        auto v = c.get();
        printf("%d\n", v);
        //////////////////////////////////////////////////////////////////////////
    }

    test_interface();

    return 0;
}

