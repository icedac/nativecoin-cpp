/****************************************************************************
 *
 *  wallet.cpp
 *      ($\nativecoin-cpp\src)
 *
 *  by icedac 
 *
 ***/
#include "stdafx.h"
#include "wallet.h"

namespace nc {

    coin wallet::get_balance(const icache<unspent_txout>& cache) const
    {
        auto address = get_address();
        auto view = cache.view();

        coin balance;

        for (const auto& tx : view) {
            if (address == tx.address)
                balance = balance + tx.amount;
        }

        return balance;
    }

    nc::string wallet::as_debug_string() const
    {
        ostringstream ss;
        ss << get_name() << ": addr[" << get_address().get() << "]";
        return ss.str();
    }

    nc::wallet wallet::from_priv_key_str(const string& name, const ecdsa_key& priv_str)
    {
        wallet w;
        w.set_name(name);
        w.priv_key_ = priv_str;
        return w;
    }

    nc::wallet wallet::from_on_the_fly(const string& name)
    {
        wallet w;
        w.set_name(name);
        w.priv_key_ = crypt::generate_private_key();
        return w;
    }

}