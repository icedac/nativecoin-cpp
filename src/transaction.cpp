/****************************************************************************
 *
 *	transaction.cpp
 *		($\nativecoin-cpp\src)
 *
 *	by icedac
 *
 ***/
#include "stdafx.h"
#include "transaction.h"

namespace nc {


    std::tuple< bool, coin > transaction::txin_validate(const txin& tx, const icache<unspent_txout>& cache) const
    {
        if (!cache.has(tx.key)) {
            std::cout << "referenced txout not found: " << tx.as_debug_string();
            return std::make_tuple(false, coin());
        }

        const auto& ref_tx = cache.get(tx.key);
        const auto& address = ref_tx.address;

        if (!crypt::verify(address, id.get(), tx.signature))
        {
            std::cout << "invalid txin signature: " << tx.signature.get() << " txid: " << id << " address: " << address.get() << "\n";
            return std::make_tuple(false, coin());
        }

        return std::make_tuple(true, ref_tx.amount);
    }

    bool transaction::sign(const ecdsa_key& priv_key, const icache<unspent_txout>& cache)
    {
        auto addr_from_key = crypt::get_address(priv_key);
        auto data = id.get();

        for (decltype(txins.size()) i = 0; i < txins.size(); ++i) {
            const auto& in = txins[i];

            const auto& out = cache.get(in.key);
            if (!out.key) {
                std::cout << "transaction::sign() : could not find ref txout\n";
                return false;
            }

            const auto& addr = out.address;
            if (out.address != addr_from_key) {
                std::cout << "transaction::sign() : address not matching\n";
                return false;
            }

            auto sig = crypt::sign(priv_key, data);
            in.signature = sig;
        }

        return true;
    }

    bool transaction::validate_coinbase(uint64 block_index) const
    {
        auto gen_id = generate_hash();
        if (gen_id != id) {
            std::cout << "[coinbase] invalid transaction id;" << id << "\n";
            return false;
        }

        if (txins.size() != 1) { // always 1
            return false;
        }

        const auto& txin = txins.front();

        if (txin.key.txout_index != block_index)
            return false;

        if (txouts.size() != 1)
            return false;

        const auto& txout = txouts.front();

        if (txout.amount != transaction::get_coinbase())
            return false;

        return true;
    }

    bool transaction::validate(const icache<unspent_txout>& cache) const
    {
        if (generate_hash() != id) {
            std::cout << "invalid transaction id;" << id << "\n";
            return false;
        }

        coin total_txin_amount;
        for (const auto& tx : txins) {
            auto t = txin_validate(tx, cache);

            if (false == std::get<0>(t)) {
                std::cout << "some of txins are invalid; " << id << "\n";
                return false;
            }
            total_txin_amount = total_txin_amount + std::get<1>(t);
        }

        coin total_txout_amount;
        for (const auto& txout : txouts) {
            total_txout_amount = total_txout_amount + txout.amount;
        }

        if (total_txin_amount != total_txout_amount) {
            std::cout << "total_txin != total tx_out; txid " << id << "\n";
        }

        return true;
    }

    string transaction::source() const
    {
        auto map_reduce_fun = [](auto& v) {
            string r;
            for (const auto& n : v)
                r += n.source();
            return r;
        };

        return map_reduce_fun(txins) + map_reduce_fun(txouts);
    }

    string transaction::as_debug_string() const
    {
        ostringstream ss;
        ss << "transaction {\n";
        ss << "\ttxins {\n";
        for (const auto& tx : txins)
            ss << "\t\t" << tx.as_debug_string() << "\n";
        ss << "\t},\n";
        ss << "\ttxouts {\n";
        for (const auto& tx : txouts)
            ss << "\t\t" << tx.as_debug_string() << "\n";
        ss << "\t},\n";
        ss << "\tid { " << id.get() << " }\n";
        ss << "}\n";
        return ss.str();
    }

    /*static*/ nc::transaction transaction::generate_coinbase(const ecdsa_address& addr, uint32 block_index)
    {
        transaction t;
        t.txins.emplace_back(txin{ "", block_index, "" });
        t.txouts.emplace_back(txout{ addr, transaction::get_coinbase() });
        t.build();
        return t;
    }

    /*static*/ nc::transaction transaction::generate_genesis(const ecdsa_address& addr)
    {
        return generate_coinbase(addr, 0);
    }

    transaction transaction::get_genesis()
    {
        transaction gen = nc::transaction::generate_genesis(
            nc::ecdsa_address{ GENESIS_PUB }
        );
        return gen;
    }

    coin transaction::get_coinbase()
    {
        return coin{ 50 };
    }
}
