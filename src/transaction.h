#pragma once
/****************************************************************************
 *
 *  transaction.h
 *      ($\nativecoin-cpp\src)
 *
 *  by icedac
 *
 ***/
#ifndef ___NATIVECOIN__NATIVECOIN__TRANSACTION_H_
#define ___NATIVECOIN__NATIVECOIN__TRANSACTION_H_

#include "nc.h"
#include "crypt.h"


namespace nc {

    template < typename T >
    class icache {
    public:
        typedef typename T::PK PK;

        virtual gsl::span<const T> view() const = 0;
        virtual bool has(const PK& k) const = 0;
        virtual const T& get(const PK& k) const = 0;
    };

    class txout_key {
    public:
        string          tx_id;
        uint32          txout_index;

        inline string source() const {
            ostringstream ss;
            ss << tx_id << txout_index;
            return ss.str();
        }
        inline string as_debug_string() const {
            ostringstream ss;
            ss << "txout_key { tx_id: " << tx_id << ", txout_index: " << txout_index << "}";
            return ss.str();
        }

        inline operator bool() const {
            if (tx_id.empty()) return false;
            return true;
        }

        inline bool operator == (const txout_key& rhs) const {
            if (tx_id == rhs.tx_id && txout_index == rhs.txout_index) return true;
            return false;
        }
    };

    // candidate for icahce<T>
    class unspent_txout {
    public:
        typedef txout_key PK;
        typedef ecdsa_address K;
        PK              key;
        ecdsa_address   address;
        coin            amount;

        inline bool operator == (const unspent_txout& rhs) const {
            if (key == rhs.key) return true;
            return false;
        }
    };

    class txin {
    public:
        typedef txout_key PK;
        PK                      key;
        mutable ecdsa_signature signature;

        inline string source() const {
            ostringstream ss;
            ss << key.source();
            return ss.str();
        }

        inline string as_debug_string() const {
            ostringstream ss;
            ss << "txin { " << key.as_debug_string() << ", signature: " << signature.get() << "}";
            return ss.str();
        }
    };

    class txout {
    public:
        ecdsa_address   address;
        coin            amount;

        inline string source() const {
            ostringstream ss;
            ss << address << amount;
            return ss.str();
        }

        inline string as_debug_string() const {
            ostringstream ss;
            ss << "txout { address: " << address.get() << ", amount: " << amount.get() << " }";
            return ss.str();
        }
    };

    // priv_key : D7CB4C0ABF38327AC93829E889906D98A7D2BD24D95DF58C72BC2AE8C168AB0E
    // pub_key: 04FAE9D499739BDCB475F746C35AA931BFF8F9448395F12C09E445546E2B9958681693E480FB6379D5F9645F970E7154359EFFCC6C8C150B04CCB86EA31B9062D3
    const char GENESIS_PRIV[] = "D7CB4C0ABF38327AC93829E889906D98A7D2BD24D95DF58C72BC2AE8C168AB0E";
    const char GENESIS_PUB[] = "04FAE9D499739BDCB475F746C35AA931BFF8F9448395F12C09E445546E2B9958681693E480FB6379D5F9645F970E7154359EFFCC6C8C150B04CCB86EA31B9062D3";

    class transaction {
    public:
        vector< txin >  txins;
        vector< txout > txouts;

        mutable sha256_hash id; //SHA256(src)

        // hashing
    public:
        string source() const;

        string as_debug_string() const;

        inline sha256_hash generate_hash() const {
            return nc::crypt::sha256(source());
        }

        inline void build() const {
            id = generate_hash();
        }

        // genesis/default things
    public:
        static transaction generate_coinbase(const ecdsa_address& addr, uint32 block_index);

        static transaction generate_genesis(const ecdsa_address& addr);

        static transaction generate() {
            transaction t;
            return t;
        }

        static transaction get_genesis();

        static coin get_coinbase();


        // methods
    public:
        // 'verify' txin signature and get coin
        std::tuple< bool, coin > txin_validate(const txin& tx, const icache<unspent_txout>& cache) const;

        // sign all txins
        bool sign(const ecdsa_key& priv_key, const icache<unspent_txout>& cache);

        // special case: Coinbase Transaction
        bool validate_coinbase(uint64 block_index) const;

        bool validate(const icache<unspent_txout>& cache) const;

    };
}


#endif// ___NATIVECOIN__NATIVECOIN__TRANSACTION_H_

