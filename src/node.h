#pragma once
/****************************************************************************
 *
 *  node.h
 *      ($\nativecoin-cpp\src)
 *
 *  by icedac 
 *
 ***/
#ifndef ___NATIVECOIN__NATIVECOIN__NODE_H_
#define ___NATIVECOIN__NATIVECOIN__NODE_H_

#include "nc.h"
#include "blockchain.h"
#include "wallet.h"

namespace nc {

    class node {
    public:
        block_chain bc;

        // transaction waiting pool
        vector< transaction > pool;

        unordered_map<string, wallet> wallets;

    public:
        void init();

        string as_debug_string( const wallet& w) const;

    public:
        const wallet& create_wallet(const string& name, const ecdsa_key& priv_key );

        const wallet& create_wallet(const string& name);

        // just give it away with name;
        const wallet& get_wallet(const string& name) const;

        // create transaction and add to pool
        bool create_transaction( const wallet& w, const ecdsa_address& receiver, coin amount );

        void validate_trans_pool();

        void commit_block();
    };
}

#endif// ___NATIVECOIN__NATIVECOIN__NODE_H_

