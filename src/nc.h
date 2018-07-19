#pragma once
/****************************************************************************
 *
 *  nc.h
 *      ($\nativecoin-cpp\src)
 *
 *  by icedac 
 *
 ***/
#ifndef ___NATIVECOIN__NATIVECOIN__NC_H_
#define ___NATIVECOIN__NATIVECOIN__NC_H_

#include <string>
#include <vector>
#include <unordered_map>
#include <set>
#include <iostream>
#include <sstream>

#define GSL_THROW_ON_CONTRACT_VIOLATION
#include <gsl/gsl>

#include <utility>
#include <stdio.h>
#include <tuple>
#include <iomanip>

#include "platform.h"

namespace nc {
    typedef unsigned char byte;
    typedef std::int32_t int32;
    typedef std::int64_t int64;
    typedef std::uint32_t uint32;
    typedef std::uint64_t uint64;
    using string = std::string;
    template < typename T >
    using vector = std::vector<T>;
    template < typename T >
    using set = std::set<T>;
    using stringstream = std::stringstream;
    using ostringstream = std::ostringstream;

    template < typename K, typename T >
    using unordered_map = std::unordered_map< K, T>;

    typedef uint32 difficulty_t;
    typedef uint64 nonce_t;
    typedef uint64 timestamp_t;
    typedef uint32 blockindex_t;
}

namespace nc {

    class coin {
    public:
        static const auto SATOSHI_PER_COIN = 1000000;
        explicit inline coin() noexcept : satoshi(0) {}
        explicit inline coin(int32 amount) noexcept : satoshi(amount*SATOSHI_PER_COIN) {}

        inline coin(const coin& c) = default;
        inline coin& operator = (const coin& rhs) = default;
        inline coin(coin&& c) = default;
        inline coin& operator = (coin&& rhs) = default;


        inline operator int32() const {
            return get();
        }

        inline coin& operator += (const coin& rhs) {
            satoshi += rhs.get_as_satoshi();
            return *this;
        }

        inline coin& operator -= (const coin& rhs) {
            satoshi -= rhs.get_as_satoshi();
            return *this;
        }

        inline coin operator + (const coin& rhs) {
            return coin::from_satoshi(this->get_as_satoshi() + rhs.get_as_satoshi());
        }
        inline coin operator - (const coin& rhs) {
            return coin::from_satoshi(this->get_as_satoshi() - rhs.get_as_satoshi());
        }

        inline bool operator == (const coin& rhs) const {
            return this->satoshi == rhs.satoshi;
        }
        inline bool operator != (const coin& rhs) const {
            return !(*this == rhs);
        }

        inline int32 get() const {
            return (int32)(satoshi / SATOSHI_PER_COIN);
        }

        inline int64 get_as_satoshi() const { return satoshi; }

        inline int64 get_satoshi() const {
            return satoshi % SATOSHI_PER_COIN;
        }

        inline string as_string() const {

            int64 satoshi = get_satoshi();

            ostringstream ss;
            ss << get() << ".";
            if (satoshi)
                ss << std::setfill('0') << std::setw(6) << satoshi;
            else
                ss << "00";

            return ss.str();
        }

        static inline coin from_satoshi(int64 satoshi) {
            coin c{ 0 };
            c.satoshi = satoshi;
            return c;
        }


    private:
        int64		satoshi;
    };
}


#endif// ___NATIVECOIN__NATIVECOIN__NC_H_
