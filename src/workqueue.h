/*
  Java JVMTI Trace Extraction
  by Romain Gaucher <r@rgaucher.info> - http://rgaucher.info

  Copyright (c) 2011-2012 Romain Gaucher <r@rgaucher.info>

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
*/
#ifndef __WORK_QUEUE_H
#define __WORK_QUEUE_H

#include <iostream>
#include <queue>

#include <boost/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition.hpp>

#include "config.h"

// stolen from the web. 
template<typename T>
struct WorkQueue {
    std::queue<T> msg;
    mutable boost::mutex mtx;
    boost::condition_variable cond_variable;

    // Safely push an element in the queue
    void push(const T& item) {
        boost::mutex::scoped_lock lock(mtx);

#ifdef DEBUG_INLINE
        std::cout << "WorkQueue::push- New item in the queue" << std::endl;
#endif

        msg.push(item);
        lock.unlock();
        cond_variable.notify_one();
    }

    bool empty() const {
        boost::mutex::scoped_lock lock(mtx);
        return msg.empty();
    }

    bool try_pop(T& item) {
        boost::mutex::scoped_lock lock(mtx);
        if(msg.empty())
            return false;
        item = msg.front();
        msg.pop();
        return true;
    }

    void wait_and_pop(T& item) {
        boost::mutex::scoped_lock lock(mtx);

#ifdef DEBUG_INLINE
        std::cout << "WorkQueue::wait_and_pop- Wait for new item..." << std::endl;
#endif
        
        while(msg.empty()) {
            cond_variable.wait(lock);
        }
        
        item = msg.front();
        msg.pop();
    }

    unsigned int size() const {
        return msg.size();
    }

};

#endif