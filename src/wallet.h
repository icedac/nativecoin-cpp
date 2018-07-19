#pragma once
/****************************************************************************
 *
 *	wallet.h
 *		($\nativecoin-cpp\src)
 *
 *	by icedac
 *
 ***/
#ifndef ___NATIVECOIN__NATIVECOIN__WALLET_H_
#define ___NATIVECOIN__NATIVECOIN__WALLET_H_

#include "nc.h"
#include "transaction.h"

namespace nc {
    // simple wallet
    class wallet {
    public:
        inline wallet() {}

        inline wallet(wallet&& rhs) noexcept {
            *this = std::move(rhs);
        }

        inline wallet& operator = (wallet&& rhs) noexcept {
            priv_key_ = std::move(rhs.priv_key_);
            name_.swap(rhs.name_);
            return *this;
        }

        inline operator bool() const {
            if (!priv_key_.get().empty()) return true;
            return false;
        }

    public:
        coin get_balance(const icache<unspent_txout>& cache) const;

        inline const ecdsa_key& get() const {
            return priv_key_;
        }

        inline ecdsa_address get_address() const {
            return crypt::get_address(priv_key_);
        }

        string as_debug_string() const;

        void to_file(const string& path) {
            //
        }

    public:
        static wallet from_file(const string& path) {
            wallet w;
            return w;
        }

        static wallet from_priv_key_str(const string& name, const ecdsa_key& priv_str);

        static wallet from_on_the_fly(const string& name);

        inline const string& get_name() const {
            return name_;
        }
        inline void set_name(const string& name) {
            name_ = name;
        }
        inline void set_name(string&& name) {
            name_.swap(name);
        }

    private:
        string      name_;
        ecdsa_key   priv_key_;
    };


}

#endif// ___NATIVECOIN__NATIVECOIN__WALLET_H_

