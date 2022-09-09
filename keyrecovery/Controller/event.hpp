//
//  event.hpp
//  keyrecovery
//
//  Created by Frederick Benjamin Woodruff on 22/05/2022.
//

#ifndef event_hpp
#define event_hpp

#include <stdio.h>
#include "global.hpp"
#include <optional>
#include <mutex>
#include <thread>
#include <atomic>
#include "blocking_queue.hpp"
#include "blockstack.hpp"
#include <variant>
/*
 GUI:
 get_event (timer/pipe or button press):
    if timer/pipe:
        read status object, and update gui
    if button:
        send message to event handler wrt. status object
 
 Terminal UI:
    if message:
        send message to event handler wrt. status object
    if status request:
        read status object and update gui

 */



struct message {
    ustring sender_ip;
    std::variant</*block, transaction, public_key,*/ size_t, action> k; // size_t is block height request
};


class cryptocurrency {
public:
    blockstack miner_state;

    int net_pipe_fd;
    std::atomic<bool> stop_token;
    
    blocking_queue<message> controller_inbox;
    blocking_queue<message> miner_inbox;
    blocking_queue<message> net_inbox;
    
    void terminal_UI();
    void graphical_UI(); //
    
    void miner_loop();
    void net_serv_loop();
    void net_cli_loop();

};

#endif /* event_hpp */
