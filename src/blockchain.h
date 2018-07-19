#pragma once
/****************************************************************************
 *
 *  blockchain.h
 *      ($\nativecoin-cpp\src)
 *
 *  by icedac
 *
 ***/
#ifndef ___NATIVECOIN__NATIVECOIN__BLOCKCHAIN_H_
#define ___NATIVECOIN__NATIVECOIN__BLOCKCHAIN_H_

#include "nc.h"
#include "crypt.h"
#include "transaction.h"

namespace nc {

    class block {
    public:
        blockindex_t            index;
        sha256_hash             prev;
        timestamp_t             timestamp;
        vector< transaction >   data;
        difficulty_t            difficulty;
        nonce_t                 nonce;

        mutable sha256_hash     hash;

        // hash
    public:
        inline string source() const {
            return source(data);
        }

        inline sha256_hash generate_hash() const {
            return generate_hash(data);
        }

        inline void build() {
            hash = generate_hash();
        }

        // hash with oustide data
    public:
        string source(gsl::span< const transaction > v) const;

        inline sha256_hash generate_hash(gsl::span< const transaction > v) const {
            return nc::crypt::sha256(source(v));
        }

    public:
        string as_debug_string() const;

        bool validate_hash() const;

        bool validate(const icache<unspent_txout>& cache) const;
        
        // retrieve tuple { spent_tx, new_unspent_tx } 
        // from transaction[]
        // result will be used for update unspent_txout
        std::tuple< vector< unspent_txout >, vector< unspent_txout > > resolve_transaction() const;

        // make new unspent_txout[] and return it
        std::tuple< bool, vector< unspent_txout > > update_unspent_txout(gsl::span< const unspent_txout > utxouts) const;

        // validate transactions[], and get new unspent_txout[]
        std::tuple< bool, vector< unspent_txout > > process(icache<unspent_txout>& cache) const;

        static block generate_genesis();

        static block create_empty() {
            block b;
            return b;
        }
    };

    /****************************************************************************
     * 	
     */
    class block_chain : public icache<unspent_txout> {
    public:
        static const auto BLOCK_GENERATION_INTERVAL = 10; // seconds

        static const auto DIFFICULTY_ADJUSTMENT_INTERVAL = 10; // blocks

        vector< block >             blocks;

        // 'vector' is very bad performance container for 'unspent_txout' but go for education purpose
        // for production, need to change at least 'unordered_map' for search complexity and remove at random position
        vector< unspent_txout >     utxouts;

        bool init();

        string debug_as_string() const;

        inline auto get() const {
            return gsl::span< const block >{ blocks };
        }

        inline auto get_unspent_txouts() const {
            return gsl::span< const unspent_txout >{ utxouts };
        }

        inline const block& get_lastest_block() const {
            return blocks.back();
        }

        bool validate_block(const block& nb) const;

        difficulty_t get_new_difficulty() const;

        difficulty_t get_difficulty() const;

        block find_block( block&& b, gsl::span< const transaction > v);

        bool add(block&& b);

        bool generate_new_block(gsl::span< const transaction > v);

        // methods; bad performnace
    public:
        //
        // unspent_txout[];
        // 
        // unordered_map< key, unspent_txout >
        // unordered_map< address, unspent_txout[] >
        inline vector<unspent_txout> cache_group_by(const ecdsa_address& address) {
            vector< unspent_txout > result;
            for (const auto& tx : utxouts) {
                if (tx.address == address)
                    result.push_back(tx);
            }
            return result;
        }

        // from icache<T>
    public:
        inline gsl::span<const unspent_txout> view() const override{
            return gsl::span< const unspent_txout >{ utxouts };
        }
        inline bool has(const unspent_txout::PK& k) const override {
            for (const auto& tx : utxouts) {
                if (tx.key == k) return true;
            }
            return false;
        }

        inline const unspent_txout& get(const unspent_txout::PK& k) const override {
            for (const auto& tx : utxouts) {
                if (tx.key == k) return tx;
            }

            static unspent_txout blank_{ {"",0}, ecdsa_address(), coin() };
            return blank_;
        }

    public:
        static bool validate_timestamp(timestamp_t nb_timestamp, timestamp_t prevb_timestamp);

    };
}

#endif// ___NATIVECOIN__NATIVECOIN__BLOCKCHAIN_H_

