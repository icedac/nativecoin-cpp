/****************************************************************************
 *
 *  node.cpp
 *      ($\nativecoin-cpp\src)
 *
 *  by icedac 
 *
 ***/
#include "stdafx.h"
#include "node.h"

namespace nc {

    //

    void node::init()
    {
        bc.init();

        const auto& genesis_wallet = create_wallet( "genesis", ecdsa_key(nc::GENESIS_PRIV) );
        const auto& node_wallet = create_wallet("node");

        std::cout << as_debug_string(genesis_wallet) << "\n";
        std::cout << as_debug_string(node_wallet) << "\n";         
    }

    nc::string node::as_debug_string(const wallet& w) const
    {
        coin balance = w.get_balance(bc);

        ostringstream ss;
        ss << w.as_debug_string() << " balance: " << balance.as_string();
        return ss.str();
    }


    const nc::wallet& node::create_wallet(const string& name, const ecdsa_key& priv_key)
    {
        auto w = wallet::from_priv_key_str(name, priv_key);
        wallets.emplace(std::make_pair(name, std::move(w)));
        return wallets.at(name);
    }

    const nc::wallet& node::create_wallet(const string& name)
    {
        auto w = wallet::from_on_the_fly(name);
        wallets.emplace(std::make_pair(name, std::move(w)));
        return wallets.at(name);
    }

    const nc::wallet& node::get_wallet(const string& name) const
    {
        auto itr = wallets.find(name);
        if (itr == wallets.end()) {
            static wallet empty_;
            return empty_;
        }

        return itr->second;
    }

    bool node::create_transaction(const wallet& w, const ecdsa_address& receiver, coin amount)
    {
        auto sender_address = w.get_address();

        set< txout_key > used;
        for (const auto& t : pool) {
            for (const auto& in : t.txins) {
                used.insert(in.key);
            }
        }

        auto sender_cache = bc.cache_group_by(sender_address);
        std::remove_if(sender_cache.begin(), sender_cache.end(), [&used](const auto& t) {
            if (used.find(t.key) != used.end()) {
                // already used
                return true;
            }
            return false;
        });

        // get enough unspent_txout for transaction
        vector<unspent_txout> use_txouts;
        coin use_amount;
        for (const auto& c : sender_cache) {
            use_txouts.push_back(c);
            use_amount += c.amount;
            if (use_amount > amount) break;
        }
        if (amount > use_amount) {
            // balance insufficient
            return false;
        }
        coin left_amount = use_amount - amount;

        auto tran = transaction::generate();

        // build txins
        for (const auto& t : use_txouts) {
            tran.txins.emplace_back(txin{ t.key });
        }

        // build txouts
        tran.txouts.emplace_back(txout{ receiver, amount });
        tran.txouts.emplace_back(txout{ sender_address, left_amount });

        tran.build();

        if (!tran.sign(w.get(), bc))
            return false;

        // add to pool
        pool.emplace_back(tran);

        return true;
    }

    void node::validate_trans_pool()
    {
        const icache<unspent_txout>& cache = bc;

        for (auto itr = pool.begin(), itrEnd = pool.end(); itr != itrEnd; ++itr) {
            const auto& tran = (*itr);
            for (const auto& tx : tran.txins) {
                if (!bc.has(tx.key)) {
                    // remove this tran
                    std::cout << "following transaction removed;\n";
                    std::cout << tran.as_debug_string();

                    itr = pool.erase(itr);
                    break; // break txins loop
                }
            }
        }
    }

    void node::commit_block()
    {
        auto coinbase_receiver = get_wallet("genesis").get_address();
        const auto& lb = bc.get_lastest_block();

        vector< transaction > new_trans{
            transaction::generate_coinbase(coinbase_receiver, lb.index + 1)
        };

        std::copy(pool.begin(), pool.end(), std::back_inserter(new_trans));

        if (bc.generate_new_block(new_trans)) {
            std::cout << "pool commited!\n";
        }
        else
        {
            std::cout << "pool commit failed!\n";
        }
    }

}