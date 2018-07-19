/****************************************************************************
 *
 *	blockchain.cpp
 *		($\nativecoin-cpp\src)
 *
 *	by icedac
 *
 ***/
#include "stdafx.h"
#include "blockchain.h"
#include "util.h"

namespace nc {

    /****************************************************************************
     *	block
     */
    bool block::validate(const icache<unspent_txout>& cache) const
    {
        if (data.empty()) {
            std::cout << "block transaction is empty\n";
            return false;
        }

        // first block is coinbase
        const auto& t = data.front();
        if (false == t.validate_coinbase(index))
        {
            std::cout << "[coinbase] invalid transaction:\n" << t.as_debug_string();
            return false;
        }

        // check dup txin
        set< string > s;
        for (const auto& t : data) {
            for (const auto& tx : t.txins) {
                auto key = tx.key.source();

                if (s.find(key) != s.end()) {
                    // found dup
                    std::cout << "key is dup; already used\n";
                    return false;
                }
                s.emplace(key);
            }
        }

        // validate other transactions
        bool skip_coinbase = true;
        for (const auto& t : data) {
            if (skip_coinbase) {
                skip_coinbase = false;
                continue;
            }

            if (false == t.validate(cache)) {
                std::cout << "transaction validate failed\n";
                return false;
            }
        }

        return true;
    }

    string block::source(gsl::span< const transaction > v) const
    {
        ostringstream ss;
        ss << index << prev << timestamp;
        for (const auto& t : v)
            ss << t.source();
        ss << difficulty;
        ss << nonce;
        return ss.str();
    }

    string block::as_debug_string() const
    {
        ostringstream ss;
        ss << "block[" << index << "] {\n";
        // ss << "\tblock_index: " << index << "\n";
        ss << "\t,hash: " << hash.get() << "\n";
        ss << "\t,prev_hash: " << prev.get() << "\n";
        for (const auto& t : data) {
            ss << util::tab_strings(t.as_debug_string());
        }
        ss << "}\n";
        return ss.str();
    }

    bool block::validate_hash() const
    {
        if (generate_hash() != hash) return false;
        if (hash.get_difficulty() < difficulty) return false;
        return true;
    }

    std::tuple< vector< unspent_txout >, nc::vector< nc::unspent_txout > > block::resolve_transaction() const
    {
        vector< unspent_txout > spent_tx;
        vector< unspent_txout > new_unspent_tx;
        for (decltype(data.size()) i = 0; i < data.size(); ++i) {
            const auto& t = data[i];

            for (const auto& tx : t.txins) {
                // spent
                spent_tx.emplace_back(unspent_txout{ tx.key, nc::ecdsa_address(), nc::coin() });
            }

            for (const auto& tx : t.txouts) {
                // new_unspent
                new_unspent_tx.emplace_back(unspent_txout{ t.id.get(), uint32(i), tx.address, tx.amount });
            }
        }

        return std::make_tuple(spent_tx, new_unspent_tx);
    }

    std::tuple< bool, nc::vector< nc::unspent_txout > > block::update_unspent_txout(gsl::span< const unspent_txout > utxouts) const
    {
#ifdef USE_CPP_17
        auto[spent_tx, new_unspent_tx] = resolve_transaction();
#else
        auto t = resolve_transaction();
        auto spent_tx = std::get<0>(t);
        auto new_unspent_tx = std::get<1>(t);
#endif
        new_unspent_tx; // will be new utxouts

        auto find_utx_from_spent_tx_fun = [&spent_tx](const auto& utx) {
            auto itr = std::find(spent_tx.begin(), spent_tx.end(), utx);
            if (itr == spent_tx.end()) return false;
            return true;
        };

        for (const auto& tx : utxouts) {
            if (!find_utx_from_spent_tx_fun(tx))
                new_unspent_tx.push_back(tx);
        }

        return std::make_tuple(true, new_unspent_tx);
    }

    std::tuple< bool, nc::vector< nc::unspent_txout > > block::process(icache<unspent_txout>& cache) const
    {
        const vector<unspent_txout> blank;

        if (!validate(cache)) {
            std::cout << "invalid block\n";
            return std::make_tuple(false, vector< unspent_txout >());
        }

        return update_unspent_txout(cache.view());
    }



    bool block_chain::init()
    {
        // build genesis block
        block b{ 0, nc::sha256_hash(), util::time_since_epoch(),{ transaction::get_genesis() }, 2, 0 };
        b.build();
        blocks.emplace_back(b);

        // build genesis utxouts
        auto t = blocks[0].process( static_cast<icache<unspent_txout>&>(*this) );
        if (!std::get<0>(t)) {
            std::cout << "genesis block creation failed\n";
            return false;
        }
        utxouts = std::get<1>(t);

        return true;
    }

    nc::string block_chain::debug_as_string() const
    {
        ostringstream ss;
        ss << "blockchain {\n";
        for (const auto& b : blocks) {
            ss << util::tab_strings(b.as_debug_string());
        }
        ss << "}\n";
        return ss.str();
    }

    /****************************************************************************
     * 	blockchain
     */

    difficulty_t block_chain::get_new_difficulty() const
    {
        Ensures(blocks.size() >= DIFFICULTY_ADJUSTMENT_INTERVAL);

        const auto& lb = get_lastest_block();
        const auto& prev_adjusted_block = blocks[blocks.size() - DIFFICULTY_ADJUSTMENT_INTERVAL];

        auto time_expected = DIFFICULTY_ADJUSTMENT_INTERVAL * BLOCK_GENERATION_INTERVAL;

        auto time_taken = lb.timestamp - prev_adjusted_block.timestamp;

        if (time_taken < time_expected / 2) {
            return prev_adjusted_block.difficulty + 1;
        }
        else if (time_taken > time_expected * 2) {
            return prev_adjusted_block.difficulty - 1;
        }

        return prev_adjusted_block.difficulty;
    }

    difficulty_t block_chain::get_difficulty() const
    {
        const auto& lb = get_lastest_block();
        if (lb.index != 0 && (lb.index % DIFFICULTY_ADJUSTMENT_INTERVAL) == 0) {
            // calculate for new difficulty
            return get_new_difficulty();
        }

        return lb.difficulty;
    }


    block block_chain::find_block(block&& b, gsl::span< const transaction > v)
    {
        while (true) {
            auto hash = b.generate_hash(v);
            if (b.difficulty < hash.get_difficulty()) {
                b.hash = hash;
                for (auto& t : v) {
                    b.data.emplace_back(t);
                }
                return b;
            }
            b.nonce += 1;
        }
    }

    bool block_chain::validate_timestamp(timestamp_t nb_timestamp, timestamp_t prevb_timestamp)
    {
        return true;
    }

    bool block_chain::validate_block(const block& nb) const
    {
        const auto& lb = get_lastest_block();
        if ((lb.index + 1) != nb.index) {
            std::cout << "blockchain::validate_block() - invalid index\n";
            return false;
        }
        if (lb.hash != nb.prev) {
            std::cout << "blockchain::validate_block() - invalid hash\n";
            return false;
        }
        if (!validate_timestamp(nb.timestamp, lb.timestamp)) {
            std::cout << "blockchain::validate_block() - invalid timestamp\n";
            return false;
        }
        if (!nb.validate_hash()) {
            std::cout << "blockchain::validate_block() - invalid hash/difficulty\n";
            return false;
        }
        return true;
    }

    bool block_chain::add(block&& b)
    {
        if (!validate_block(b)) {
            return false;
        }

        auto r = b.process(*this);

        if (!std::get<0>(r)) {
            std::cout << "block is invalid\n";
            return false;
        }

        blocks.emplace_back(std::move(b));
        utxouts.swap(std::get<1>(r));

        // update transaction pool
        return true;
    }

    bool block_chain::generate_new_block(gsl::span< const transaction > v)
    {
        const auto& lb = get_lastest_block();

        block nb = block::create_empty();
        nb.index = lb.index + 1;
        nb.prev = lb.hash;
        nb.timestamp = util::time_since_epoch();
        nb.difficulty = get_difficulty();
        nb.nonce = 0;

        auto new_block = find_block(std::move(nb), v);

        if (add(std::move(new_block))) {
            // brastcastLasted?
            return true;
        }

        return false;
    }

    //////////////////////////////////////////////////////////////////////////
    //template < typename T >
    //class icache {
    //public:
    //    typedef typename T::K K;
    //    gsl::span<const T> view() const;
    //    const T& get(const K& k) const;
    //};

    // unspent_txout[]
    // single thread only

    /*
    class block_chain_cache : public icache<unspent_txout>
    public:
        struct utxout {
            string      tx_id;
            uint32      tx_index;
            coin        amount;
        };

        struct cache {
            vector< utxout >    utxouts;
            coin                balance;

            std::tuple< bool, const utxout >
                query(string tx_id, uint32 tx_index) const {
                for (const auto& utx : utxouts) {
                    if (utx.tx_id == tx_id && utx.tx_index == tx_index) {
                        utxout copied_utxout(utx);
                        return std::make_tuple(true, copied_utxout);
                    }
                }
                return std::make_tuple(false, utxout());
            }
        };

        // build/modify
    public:
        inline void queue_add(string tx_id, uint32 tx_index, ecdsa_address address, coin amount) {
            pending_to_add_.emplace_back(unspent_txout{ tx_id, tx_index, address, amount });
        }

        inline void queue_del(string tx_id, uint32 tx_index, ecdsa_address address, coin amount) {
            pending_to_del_.emplace_back(unspent_txout{ tx_id, tx_index, address, amount });
        }

        // update pending -> caches
        inline void commit() {
            // apply pending_to_del_

            // apply pending_to_add_
        }

        bool add_block(const block& nb) {
            // nb.validate(...)

            for (const auto& t : nb.data) {

                if (!add_transaction(t)) return false;
            }

            return true;
        }

        bool add_transaction(const transaction& tran) {
            // tran.validate()

            // del from cache
            for (const auto& tx : tran.txins) {
                //txtxout_id;
            }

            // add to cache
            tran.txouts;

            return true;
        }

        // rebuild
        bool rebuild(gsl::span< const block > v) {
            // clear

            for (const auto& b : v)
                if (!add_block(b)) return false;

            return true;
        }

        // query
    public:
        const cache* query(const ecdsa_address& address) const {
            auto itr = caches_.find(address);
            if (itr == caches_.end()) {
                return nullptr;
            }

            return &(itr->second);
        }

    private:
        vector< unspent_txout >                 pending_to_add_;
        vector< unspent_txout >                 pending_to_del_;

        unordered_map< ecdsa_address, cache >   caches_;
    };
    */

}

