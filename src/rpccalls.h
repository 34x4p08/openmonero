//
// Created by mwo on 13/04/16.
//


#ifndef CROWXMR_RPCCALLS_H
#define CROWXMR_RPCCALLS_H

#include "monero_headers.h"

#include <mutex>

namespace xmreg
{

    using namespace cryptonote;
    using namespace crypto;
    using namespace std;


    class rpccalls
    {
        string deamon_url ;
        uint64_t timeout_time;

        std::chrono::milliseconds timeout_time_ms;

        epee::net_utils::http::url_content url;

        epee::net_utils::http::http_simple_client m_http_client;
        std::mutex m_daemon_rpc_mutex;

        string port;

    public:

        rpccalls(string _deamon_url = "http:://127.0.0.1:18081",
                 uint64_t _timeout = 200000)
        : deamon_url {_deamon_url}, timeout_time {_timeout}
        {
            epee::net_utils::parse_url(deamon_url, url);

            port = std::to_string(url.port);

            timeout_time_ms = std::chrono::milliseconds {timeout_time};

            m_http_client.set_server(deamon_url);
        }

        bool
        connect_to_monero_deamon()
        {
            if(m_http_client.is_connected())
            {
                return true;
            }

            return m_http_client.connect(timeout_time_ms);
        }

        uint64_t
        get_current_height()
        {
            COMMAND_RPC_GET_HEIGHT::request   req;
            COMMAND_RPC_GET_HEIGHT::response  res;

            std::lock_guard<std::mutex> guard(m_daemon_rpc_mutex);

            if (!connect_to_monero_deamon())
            {
                cerr << "get_current_height: not connected to deamon" << endl;
                return false;
            }

            bool r = epee::net_utils::invoke_http_json(
                    "/getheight",
                    req, res, m_http_client, timeout_time_ms);

            if (!r)
            {
                cerr << "Error connecting to Monero deamon at "
                     << deamon_url << endl;
                return 0;
            }
            else
            {
                cout << "rpc call /getheight OK: " << endl;
            }

            return res.height;
        }

        bool
        get_mempool(vector<tx_info>& mempool_txs) {

            COMMAND_RPC_GET_TRANSACTION_POOL::request  req;
            COMMAND_RPC_GET_TRANSACTION_POOL::response res;

            std::lock_guard<std::mutex> guard(m_daemon_rpc_mutex);

            if (!connect_to_monero_deamon())
            {
                cerr << "get_current_height: not connected to deamon" << endl;
                return false;
            }

            bool r = epee::net_utils::invoke_http_json(
                    "/get_transaction_pool",
                    req, res, m_http_client, timeout_time_ms);

            if (!r)
            {
                cerr << "Error connecting to Monero deamon at "
                     << deamon_url << endl;
                return false;
            }


            mempool_txs = res.transactions;

            return true;
        }


        bool
        get_random_outs_for_amounts(const vector<uint64_t>& amounts,
                                    const uint64_t& outs_count,
                                    vector<COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::outs_for_amount>& found_outputs,
                                    string& error_msg)
        {
            COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::request  req;
            COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::response res;

            req.outs_count = outs_count;
            req.amounts = amounts;

            std::lock_guard<std::mutex> guard(m_daemon_rpc_mutex);

            if (!connect_to_monero_deamon())
            {
                cerr << "get_current_height: not connected to deamon" << endl;
                return false;
            }

            bool r = epee::net_utils::invoke_http_bin(
                    "/getrandom_outs.bin",
                    req, res, m_http_client, timeout_time_ms);


            if (!r || res.status == "Failed")
            {
                error_msg = "rpc call to /getrandom_outs.bin failed for some reason";

                cerr << "rpc call to /getrandom_outs.bin failed for some reason" << endl;
                return false;
            }

            if (!res.outs.empty())
            {
                found_outputs = res.outs;
                return true;
            }

            return false;

        }


        bool
        commit_tx(const string& tx_blob, string& error_msg)
        {
            COMMAND_RPC_SEND_RAW_TX::request  req;
            COMMAND_RPC_SEND_RAW_TX::response res;

            req.tx_as_hex = tx_blob;

            req.do_not_relay = false;

            std::lock_guard<std::mutex> guard(m_daemon_rpc_mutex);

            if (!connect_to_monero_deamon())
            {
                cerr << "get_current_height: not connected to deamon" << endl;
                return false;
            }

            bool r = epee::net_utils::invoke_http_json(
                    "/sendrawtransaction", req, res,
                    m_http_client, timeout_time_ms);

            if (!r || res.status == "Failed")
            {
                error_msg = res.reason;

                cerr << "Error sending tx: " << res.reason << endl;
                return false;
            }

            return true;
        }


        bool
        commit_tx(tools::wallet2::pending_tx& ptx, string& error_msg)
        {

            string tx_blob = epee::string_tools::buff_to_hex_nodelimer(
                    tx_to_blob(ptx.tx)
            );

            return commit_tx(tx_blob, error_msg);
        }
    };


}



#endif //CROWXMR_RPCCALLS_H
