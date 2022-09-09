//
//  event.cpp
//  keyrecovery
//
//  Created by Frederick Benjamin Woodruff on 22/05/2022.
//

#include "event.hpp"
#include "global.hpp"
#include "blocks.hpp"
#include "blocking_queue.hpp"
#include "blockstack.hpp"
#include "signatures.hpp"
#include "transaction.hpp"

#include <atomic>
#include <memory>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <optional>
#include <queue>
#include <iostream>






/*
// entry point
void cryptocurrency::terminal_UI() {
    miner_state.accounts = &interface;
    
    
    std::thread thr_miner(&cryptocurrency::miner_loop, this);
    std::thread thr_net_cli(&cryptocurrency::net_cli_loop,this);
    std::thread thr_net_serv(&cryptocurrency::net_serv_loop, this);

    
    for(;;) {
        // save pubkey data to file
        std::cout << "Selection option number:\n"
        "1. new account \n"
        "2. query balance\n" // index + recent
        "4. send coins\n"
        "5. add peer\n"
        "6. shutdown\n";
        
        std::string input;
        std::cin >> input;
        if(input == "1") {
            // generate key pair
            account_status acct {};
            auto [sk, pk] = random_keypair();
            acct.account = pk;
            acct.account_key = sk;
            
            {
                std::scoped_lock lk(interface.mut);
                interface.accounts.push_back(acct); // assert not already in there (entropy)
            }
            message msg;
            msg.k = acct.account;
            msg.sender_ip = {0};
            miner_inbox.push(msg);
            
            
            
            std::cout << "account passkey is " << priv_to_b58(*acct.account_key) << std::endl;
            std::cout << "account address is " << pub_to_b58(acct.account)       << std::endl;
        }
        if(input == "2") {
            std::cout << "passkey: ";
            std::string privkey;
            std::cin >> privkey;
            std::optional<private_key> sk; // b58 convert privkey.
            if(!sk) {
                std::cout << "invalid privkey";
                continue;
            }
            
            account_status acct {};
            acct.account = make_pubkey(*sk);
            acct.account_key = sk;
            {
                std::scoped_lock lk(interface.mut);
                interface.accounts.push_back(acct); // check if already in there
            }
            
            message msg;
            msg.sender_ip = {0};
            msg.k = acct.account;
            miner_inbox.push(msg);
            stop_token = true;
            
            std::cout << "preparing account: " << pub_to_b58(acct.account) << std::endl;
        }
        if(input == "3") {
            
            
            std::vector<account_status> copy;
            {
                std::scoped_lock lk(interface.mut);
                copy = interface.accounts;
            }
            // display balances for accounts
                // display recent incoming transactions for accounts
                // display recent outgoing transactions for accounts
        }
            
        if(input == "4") {
            std::vector<account_status> copy;
            {
                std::scoped_lock lk(interface.mut);
                copy = interface.accounts;
            }
            
            std::cout << "sender address: ";
            std::string pubkey_sender;
            std::cin >> pubkey_sender;
            
            public_key pk; // b58 pubkey_sender;
            account_status st {};
            for(account_status c : copy) {
                if(c.account == pk) {
                    st = c;
                    break;
                }
            }
            if(st.searched == false) {
                std::cout << "account not ready" << std::endl;
                continue;
            }
            if(st.balance == 0) {
                std::cout << "no confirmed coins at address" << std::endl;
                continue;
            }
            
            std::cout << "recipient address: ";
            std::string pubkey;
            std::cin >> pubkey;
            
            std::cout << "amount: ";
            std::string amount;
            std::cin >> amount;
            
            std::cout << "fee: ";
            std::string fee;
            std::cin >> fee;
            
            tx_output out;
            out.pubkey = *b58_to_pub(pubkey); // assert
            out.amount = std::stoi(amount);
            
            transaction tx = *generate_transaction(st, {out}, std::stoi(fee));
            
            miner_inbox.push({{0},tx});
            stop_token = true;
            
            std::cout << "sent to network";
        }
        if(input == "5") {
            miner_inbox.push({0,action::shutdown});
            net_inbox.push({0,action::shutdown});
            //net_pipe_fd;
            break;
        }
        if(input == "6") {
            std::cout << "Who is your friend?";
            std::string peer_ip;
            std::cin >> peer_ip;
            // add to peer list
            // refresh for peer list.
        }
        
    }
    thr_miner.join();
    thr_net_cli.join();
    thr_net_serv.join();
}


*/

/*

void cryptocurrency::miner_loop() {
    
    
    
    
    bool do_mining = false;
    std::optional<message> msg;
    
    
    // al
    
    for(;;) {
        if(do_mining) {
            msg = miner_inbox.try_pop();
        } else {
            msg = miner_inbox.pop();
        }
       
        //auto redistribution = miner_state.verify_pool();
        //for(auto msg : redistribution) {
            //net_inbox.push(msg);
        //}
        // verify mempool and create/reuse a block. any tx new, forward to peers
        
        stop_token = false;
        
        if(do_mining) {
            auto opt_block = miner_state.do_some_mining(stop_token);
            if(opt_block) {
                //net_inbox.push(*opt_block);
                std::this_thread::yield();
                //miner_state.update_for_block(*opt_block);
                // update interface object for block
                break;
            }
        }
        std::this_thread::yield(); // ?
    }
}

*/


void cryptocurrency::net_cli_loop() {
    /*
    for(;;) {
        auto msg = net_inbox.pop();
        switch (msg.k) {
            case message_type::block:
            case message_type::transaction:
                // send to peers
                break;
            case message_type::UI_shutdown:
                return;
            default:
                break;
        }
    }
     */
}

void cryptocurrency::net_serv_loop() {
    for(;;) {
        // get message from net.
        message msg; // get from net, poll block. msg might be a pipe, i.e. UI_shutdown
        /*
        if(msg.k == message_type::UI_shutdown) {
            return;
        }
         */
        miner_inbox.push(msg);
    }
}




