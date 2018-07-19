#pragma once
/****************************************************************************
 *
 *  type.h
 *      ($\nativecoin-cpp\src)
 *
 *  by icedac
 *
 ***/
#ifndef ___NATIVECOIN__NATIVECOIN__TYPE_H_
#define ___NATIVECOIN__NATIVECOIN__TYPE_H_

#include "nc.h"
#include "util.h"

namespace nc {

    class string_holder {
    public:
        inline string_holder(const string& rhs) noexcept : value(rhs) {}
        inline string_holder& operator = (const string& rhs) noexcept { value = rhs; return *this;}
        inline string_holder(string&& rhs) noexcept : value(std::move(rhs)) {}
        inline string_holder& operator = (string&& rhs) noexcept { value = std::move(rhs); return *this; }

    public:
        inline string_holder() = default;
        inline string_holder(const string_holder& rhs) = default;
        inline string_holder& operator = (const string_holder& rhs) = default;
        inline string_holder(string_holder&& rhs) noexcept : value(std::move(rhs.value)) {}
        inline string_holder& operator = (string_holder&& rhs) noexcept { value = std::move(rhs.value); return *this; }

        inline operator bool() const noexcept {
            if (value.empty()) return false;
            return true;
        }

        inline bool operator == (const string_holder& rhs) const {
            if (value == rhs.value) return true;
            return false;
        }

        inline const string& get() const noexcept { return value; }

    private:
        string value;
    };

    typedef string_holder ecdsa_key;

    class ecdsa_address {
    public:
        inline ecdsa_address(const string& rhs) noexcept : value(rhs) {}
        inline ecdsa_address& operator = (const string& rhs) noexcept { value = rhs; return *this;}
        inline ecdsa_address(string&& rhs) noexcept : value(std::move(rhs)) {}
        inline ecdsa_address& operator = (string&& rhs) noexcept { value = std::move(rhs); return *this; }

    public:
        inline ecdsa_address() = default;
        inline ecdsa_address(const ecdsa_address& rhs) = default;
        inline ecdsa_address& operator = (const ecdsa_address& rhs) = default;
        inline ecdsa_address(ecdsa_address&& rhs) noexcept : value(std::move(rhs.value)) {}
        inline ecdsa_address& operator = (ecdsa_address&& rhs) noexcept { value = std::move(rhs.value); return *this; }

        inline operator bool() const noexcept {
            if (value.empty()) return false;
            return true;
        }

        inline bool operator == (const ecdsa_address& rhs) const {
            if (value == rhs.value) return true;
            return false;
        }

        inline const string& get() const noexcept { return value; }

        // ECDSA public key size: '04' + X(64 bytes) + Y(64 bytes)
        // size in string: 2 + 64 + 64 = 130 bytes
        // size in bytes: 65 bytes
        inline bool validate() const noexcept {
            if (value.size() != 130) return false;
            // check start with '04'
            return util::is_hexstring(value);
        }

    private:
        string value;
    };

    class ecdsa_signature {
    public:
        inline ecdsa_signature(const string& rhs) noexcept : value(rhs) {}
        inline ecdsa_signature& operator = (const string& rhs) noexcept { value = rhs; return *this;}
        inline ecdsa_signature(string&& rhs) noexcept : value(std::move(rhs)) {}
        inline ecdsa_signature& operator = (string&& rhs) noexcept { value = std::move(rhs); return *this; }

    public:
        inline ecdsa_signature() = default;
        inline ecdsa_signature(const ecdsa_signature& rhs) = default;
        inline ecdsa_signature& operator = (const ecdsa_signature& rhs) = default;
        inline ecdsa_signature(ecdsa_signature&& rhs) noexcept : value(std::move(rhs.value)) {}
        inline ecdsa_signature& operator = (ecdsa_signature&& rhs) noexcept { value = std::move(rhs.value); return *this; }

        inline operator bool() const noexcept {
            if (value.empty()) return false;
            return true;
        }

        inline bool operator == (const ecdsa_signature& rhs) const {
            if (value == rhs.value) return true;
            return false;
        }

        inline const string& get() const noexcept { return value; }

    private:
        string value;
    };

    class sha256_hash {
    private:
        inline sha256_hash(const string& rhs) noexcept : value(rhs) {}
        inline sha256_hash& operator = (const string& rhs) noexcept { value = rhs; return *this;}
        inline sha256_hash(string&& rhs) noexcept : value(rhs) {}
        inline sha256_hash& operator = (string&& rhs) noexcept { value = rhs; return *this;}

    public:
        inline sha256_hash() = default;
        inline sha256_hash(const sha256_hash& rhs) = default;
        inline sha256_hash& operator = (const sha256_hash& rhs) = default;
        inline sha256_hash(sha256_hash&& rhs) noexcept : value(std::move(rhs.value)) {}
        inline sha256_hash& operator = (sha256_hash&& rhs) noexcept { value = std::move(rhs.value); return *this; }

        inline operator bool() const noexcept {
            if (value.empty()) return false;
            return true;
        }

        inline bool operator == (const sha256_hash& rhs) const {
            if (value == rhs.value) return true;
            return false;
        }

        inline string get() const noexcept { return value; }

        inline difficulty_t get_difficulty() const noexcept { 
            difficulty_t d = 0;
            for (char c : value) {
                if (c == '0') d += 1;
                else break;
            }
            return d;
        }

        inline static sha256_hash from_string(string&& hash_string) noexcept {
            return sha256_hash{ std::move(hash_string) };
        }
        inline static sha256_hash from_string(const string& hash_string) noexcept {
            return sha256_hash{ hash_string };
        }

    private:
        string value;
    };

}

namespace std {

    template <>
    struct hash<nc::ecdsa_address>
    {
        std::size_t operator()(const nc::ecdsa_address& k) const
        {
            using std::hash;
            using std::string;

            return hash<string>()(k.get());
        }
    };

}

#endif// ___NATIVECOIN__NATIVECOIN__TYPE_H_

