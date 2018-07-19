/****************************************************************************
 *
 *	crypt.cpp
 *		($\nativecoin-cpp\src)
 *
 *	by icedac
 *
 ***/
#include "stdafx.h"
#include "nc.h"
#include "crypt.h"
#include "util.h"
#include <mutex>

#include <openssl/evp.h>
#include <openssl/ecdsa.h>
#include <openssl/bn.h>

#ifdef _WIN64
#pragma comment( lib, "libeay32_x64.lib" )
#pragma comment( lib, "ssleay32_x64.lib" )
#else
#pragma comment( lib, "libeay32_x86.lib" )
#pragma comment( lib, "ssleay32_x86.lib" )
#endif

namespace nc {

    namespace crypt {

        // openssl crypto library c++ wrapper
        // manual: https://www.openssl.org/docs/man1.0.2/crypto/
        //			very clumpsy and almost no example :-(
        namespace internal {

            // msg_digest_ctx
            class md_ctx {
            public:
                friend class md;

                md_ctx() {
                    ctx_ = EVP_MD_CTX_create();
                }

                ~md_ctx() {
                    if (ctx_)
                    {
                        EVP_MD_CTX_destroy(ctx_);
                    }
                }

                md_ctx(const md_ctx&) = delete;
                md_ctx& operator = (const md_ctx&) = delete;
                md_ctx(md_ctx&& rhs) noexcept : ctx_(rhs.ctx_) {
                    rhs.ctx_ = nullptr;
                }
                md_ctx& operator = (md_ctx&& rhs) noexcept {
                    ctx_ = rhs.ctx_;
                    rhs.ctx_ = nullptr;
                }

            public:
                void update(gsl::span< const byte > v) {
                    EVP_DigestUpdate(ctx_, v.data(), v.size());
                }

                auto get() {
                    vector< byte > hash(EVP_MAX_MD_SIZE);
                    unsigned int len = 0;

                    EVP_DigestFinal_ex(ctx_, const_cast<byte*>(hash.data()), &len);

                    hash.resize(len);

                    return hash;
                }

            private:
                EVP_MD_CTX* ctx_ = nullptr;
            };

            // msg_digest
            class md {
            public:
                md() {}
                md(const md&) = delete;
                md& operator = (const md&) = delete;
                md(md&& rhs) noexcept : md_(rhs.md_) {
                    rhs.md_ = nullptr;
                }
                md& operator = (md&& rhs) noexcept {
                    md_ = rhs.md_;
                    rhs.md_ = nullptr;
                }

                md_ctx create_ctx() {
                    md_ctx c;
                    EVP_DigestInit_ex(c.ctx_, md_, nullptr);
                    return c;
                }

                operator bool() const {
                    if (md_) return true;
                    return false;
                }

            public:
                static md from_name(string md_name) {
                    md m;
                    m.md_ = EVP_get_digestbyname(md_name.c_str());
                    return m;
                }

            private:
                const EVP_MD* md_ = nullptr;
            };

            // ec_key
            //
            //	BIGNUM used for Private Key
            //	EC_POINT used for Public Key and can be caculated from Private Key
            //		@see load_private_key_from_hex() for calculation
            class ec_key {
            public:
                enum ec_curve_name_t {
                    SECP256K1 = NID_secp256k1
                };

                ec_key() {}
                ~ec_key() {
                    if (ec_)
                        EC_KEY_free(ec_);
                }

                ec_key(const ec_key&) = delete;
                ec_key& operator = (const ec_key&) = delete;

                ec_key(ec_key&& rhs) noexcept : ec_(rhs.ec_) {
                    rhs.ec_ = nullptr;
                }
                ec_key& operator = (ec_key&& rhs) noexcept {
                    ec_ = rhs.ec_;
                    rhs.ec_ = nullptr;
                }

                bool generate_key() {
                    if (!EC_KEY_generate_key(ec_)) return false;
                    return true;
                }

                auto get_private_key() const {
                    const BIGNUM* priv = EC_KEY_get0_private_key(ec_);

                    char* priv_key = BN_bn2hex(priv);
                    // printf("priv: %s\n", priv_key);
                    std::string priv_str = priv_key;
                    OPENSSL_free(priv_key);

                    return priv_str;
                }

                auto get_public_key() const {
                    const EC_POINT* pub = EC_KEY_get0_public_key(ec_);

                    char* pub_key = EC_POINT_point2hex(EC_KEY_get0_group(ec_), pub, POINT_CONVERSION_UNCOMPRESSED, NULL);
                    // printf("pub: %s\n", pub_key);
                    std::string pub_str = pub_key;
                    OPENSSL_free(pub_key);

                    return pub_str;
                }

                bool load_public_key_from_hex(string hex) {

                    const auto* group = EC_KEY_get0_group(ec_);
                    BN_CTX* ctx = BN_CTX_new();

                    EC_POINT* pub = EC_POINT_new(group);

                    EC_POINT_hex2point(group, hex.c_str(), pub, ctx);
                    EC_KEY_set_public_key(ec_, pub);

                    // FIXME:	donno ownership transfer or not from EC_KEY_set_public_key(), so it can be crash
                    //			openssl documentation is clumpsy - https://www.openssl.org/docs/man1.0.2/crypto/EC_KEY_set_public_key.html
                    EC_POINT_free(pub);

                    BN_CTX_free(ctx);
                    return true;
                }

                // also calculate pub key and set it
                bool load_private_key_from_hex(string hex) {
                    BIGNUM start;
                    BN_init(&start);

                    BN_CTX* ctx = BN_CTX_new();
                    BIGNUM* priv = &start;

                    BN_hex2bn(&priv, hex.c_str());
                    const auto* group = EC_KEY_get0_group(ec_);

                    EC_POINT* pub = EC_POINT_new(group);

                    EC_KEY_set_private_key(ec_, priv);

                    if (!EC_POINT_mul(group, pub, priv, nullptr, nullptr, ctx))
                    {
                        std::cout << "EC_POINT_mul() failed\n";
                        return false;
                    }

                    EC_KEY_set_public_key(ec_, pub);

                    // FIXME:	donno ownership transfer or not from EC_KEY_set_public_key(), so it can be crash
                    //			openssl documentation is clumpsy - https://www.openssl.org/docs/man1.0.2/crypto/EC_KEY_set_public_key.html
                    EC_POINT_free(pub);

                    BN_CTX_free(ctx);

                    return true;
                }

                // sign()
                //	
                //		sign data and return DER encoded
                //
                vector<byte> sign(gsl::span< const byte > v) {
                    // FIXME: need to check and manage ownership of 'sig'

                    ECDSA_SIG* sig;
                    sig = ECDSA_do_sign(v.data(), (int)v.size(), ec_);
                    if (!sig) {
                        return vector< byte >();
                    }

                    vector< byte > der_sign(128);
                    int len = i2d_ECDSA_SIG(sig, nullptr);
                    der_sign.reserve(len + 1);
                    byte* der_sign_buf = const_cast<byte*>(der_sign.data());

                    len = i2d_ECDSA_SIG(sig, (byte**)&der_sign_buf);

                    der_sign.resize(len);

                    return der_sign;
                }

                // verify
                //
                //		verify from data and DER encoded
                //
                bool verify(gsl::span< const byte > v, vector<byte> der_sign) {
                    const byte* der_sign_buf = der_sign.data();

                    ECDSA_SIG* sig = ECDSA_SIG_new();

                    ECDSA_SIG* ok_sig = d2i_ECDSA_SIG(&sig, &der_sign_buf, (int)der_sign.size());
                    if (!ok_sig) {
                        auto der_sign_str = util::binary_to_hexstring(der_sign);
                        std::cout << "wrong signature: " << der_sign_str << "\n";
                        return false;
                    }

                    int ret = ECDSA_do_verify(v.data(), (int)v.size(), ok_sig, ec_);
                    ECDSA_SIG_free(sig);

                    if (ret == 0) {
                        std::cout << "incorrect signature\n";
                        return false;
                    }
                    else if (ret == 1)
                    {
                        return true;
                    }

                    std::cout << "error: ret: " << ret << "\n";
                    return false;
                }

            public:
                static ec_key from_curve(ec_curve_name_t ec_curve_name) {
                    ec_key key;
                    key.ec_ = EC_KEY_new_by_curve_name((int)ec_curve_name);
                    return key;
                }
            private:

                EC_KEY* ec_ = nullptr;
            };

            void run_at_exit() {
                EVP_cleanup();
            }

            static std::once_flag s_openssl_init;

            void init_main__() {

                OpenSSL_add_all_digests();

                using internal::ec_key;

                if (0)
                {
                    ec_key ec = ec_key::from_curve(ec_key::SECP256K1);
                    ec.generate_key();
                    auto pub_str = ec.get_public_key();
                    std::cout << "pub_key: " << pub_str << "\n";

                    auto priv_str = ec.get_private_key();
                    std::cout << "priv_key: " << priv_str << "\n";
                }

                // priv_key : D7CB4C0ABF38327AC93829E889906D98A7D2BD24D95DF58C72BC2AE8C168AB0E
                // pub_key: 04FAE9D499739BDCB475F746C35AA931BFF8F9448395F12C09E445546E2B9958681693E480FB6379D5F9645F970E7154359EFFCC6C8C150B04CCB86EA31B9062D3
                if (0)
                {
                    ec_key ec = ec_key::from_curve(ec_key::SECP256K1);
                    ec.load_private_key_from_hex("D7CB4C0ABF38327AC93829E889906D98A7D2BD24D95DF58C72BC2AE8C168AB0E");
                    // ec.load_public_key_from_hex("04FAE9D499739BDCB475F746C35AA931BFF8F9448395F12C09E445546E2B9958681693E480FB6379D5F9645F970E7154359EFFCC6C8C150B04CCB86EA31B9062D3");
                    auto pub_str = ec.get_public_key();
                    std::cout << "pub_key: " << pub_str << "\n";
                    // priv_str = ec2.get_private_key();
                    // std::cout << "priv_key: " << priv_str << "\n";

                    auto str = string("12345678901234567890");
                    const byte* data = reinterpret_cast<const byte*>(str.data());

                    auto der_sign = ec.sign({ data, data + str.size() });
                    auto der_sign_str = util::binary_to_hexstring(der_sign);

                }

                // high-level
                if (0)
                {
                    // sign
                    auto priv_key = nc::ecdsa_key("D7CB4C0ABF38327AC93829E889906D98A7D2BD24D95DF58C72BC2AE8C168AB0E");
                    auto str = string("12345678901234567890");

                    auto der_sign_str = nc::crypt::sign(priv_key, str);

                    // verify
                    auto pub_key = "04FAE9D499739BDCB475F746C35AA931BFF8F9448395F12C09E445546E2B9958681693E480FB6379D5F9645F970E7154359EFFCC6C8C150B04CCB86EA31B9062D3";

                    bool verified = nc::crypt::verify(ecdsa_address(pub_key), str, der_sign_str);
                    if (verified) {
                        std::cout << "[VERIFIED] ok\n";
                    }
                    else
                    {
                        std::cout << "[<FAILED> to VERIFY]\n";
                    }

                }

                std::atexit(nc::crypt::internal::run_at_exit);
            }

            void init_() {
                std::call_once(s_openssl_init, []() {
                    init_main__();
                });
            }
        } // namespace internal {
    }

    namespace crypt {

        sha256_hash sha256(string str)
        {
            internal::init_();
            using internal::md;

            auto md_sha256 = md::from_name("sha256");
            if (!md_sha256) return nc::sha256_hash::from_string("md_sha256 not found");
            auto ctx = md_sha256.create_ctx();
            const byte* data = reinterpret_cast<const byte*>(str.data());
            ctx.update({ data, data + str.size() });
            auto hash = ctx.get();

            return nc::sha256_hash::from_string(util::binary_to_hexstring(hash));
        }

        ecdsa_key generate_private_key()
        {
            using internal::ec_key;

            ec_key ec = ec_key::from_curve(ec_key::SECP256K1);
            if (!ec.generate_key()) return "";

            return ec.get_private_key();
        }

        ecdsa_address get_address(const ecdsa_key& priv_key)
        {
            using internal::ec_key;

            ec_key ec = ec_key::from_curve(ec_key::SECP256K1);
            ec.load_private_key_from_hex(priv_key.get());

            return nc::ecdsa_address{ ec.get_public_key() };
        }

        ecdsa_signature sign(const ecdsa_key& priv_key, const string& str)
        {
            using internal::ec_key;

            ec_key ec = ec_key::from_curve(ec_key::SECP256K1);
            ec.load_private_key_from_hex(priv_key.get());

            const byte* data = reinterpret_cast<const byte*>(str.data());
            auto der_sign = ec.sign({ data, data + str.size() });

            auto der_sign_str = util::binary_to_hexstring(der_sign);

            return der_sign_str;
        }

        bool verify(const ecdsa_address& addr, const string& str, const ecdsa_signature& signature)
        {
            using internal::ec_key;

            ec_key ec = ec_key::from_curve(ec_key::SECP256K1);
            ec.load_public_key_from_hex(addr.get());

            const byte* data = reinterpret_cast<const byte*>(str.data());
            auto der_sign = util::hexstring_to_binary(signature.get());

            return ec.verify({ data, data + str.size() }, der_sign);
        }

    }
}

