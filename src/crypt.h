#pragma once
/****************************************************************************
 *
 *	crypt.h
 *		($\nativecoin-cpp\src)
 *
 *	by icedac
 *
 ***/
#ifndef _____NATIVECOIN__CRYPT_H_
#define _____NATIVECOIN__CRYPT_H_

#include "nc.h"
#include "type.h"

namespace nc {

    // make it simple string-based api for education purpose
    namespace crypt {

        // SHA256
        sha256_hash sha256(string data);

        // ECDSA: curve=SECP256K1
        ecdsa_key generate_private_key();

        ecdsa_address get_address(const ecdsa_key& priv_key);

        ecdsa_signature sign(const ecdsa_key& priv_key, const string& str);

        bool verify(const ecdsa_address& addr, const string& str, const ecdsa_signature& signature);

    }
}


#endif// _____NATIVECOIN__CRYPT_H_

