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
#ifndef __STORE_H
#define __STORE_H

#include <iostream>
#include <map>
#include <string>
#include <vector>
#include <list>

#include <boost/tuple/tuple.hpp>
#include <boost/tuple/tuple_comparison.hpp>

#include <boost/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition.hpp>

#include <jvmti.h>

#include "config.h"
#ifdef USE_DATABASE
    #include "database.h"
#endif

#include "workqueue.h"

typedef boost::tuple<unsigned int, unsigned int, unsigned int> TupleFQN;
typedef std::map<std::string, unsigned int> KeyCache;
typedef std::map<TupleFQN, unsigned int> TupleKeyCache;

typedef boost::tuple<std::string, std::string, std::string, std::string, unsigned long long, unsigned long long> TupleQueueElement;


static boost::condition element_available;
static boost::mutex worker_mutex;


// Store the traces in the SQLite DB
// TOOD: refactor in a push/pop interface ot capture the actually structure of the traces
//       (call stacks) in the DB
struct TraceStore {
    bool running;
    WorkQueue<TupleQueueElement> queue;
    boost::thread thread_worker;


#ifdef USE_DATABASE
    db::Database database;
#endif

    bool start_recording;
    unsigned long long trace_id;

    KeyCache thread_cache;
    KeyCache class_cache;
    KeyCache method_cache;
    KeyCache signature_cache;
    TupleKeyCache fqn_cache;

    std::map<std::string, bool> serializable;
    std::map<std::string, bool> filter_cache;
    std::list<std::string> class_whitelist;
    std::list<std::string> class_blacklist;

    // Stack of FQN per thread
    std::map<unsigned int, TupleFQN> current_fqn;

    TraceStore() 
     : running(true), start_recording(false) {
    }


     ~TraceStore() {
#ifdef DEBUG_INLINE
        std::cout << "TraceStore::~TraceStore- Wait for worker thread to finish its job" << std::endl;
#endif
        thread_worker.join();
        dump();
    }

    void wait_threads() {
        running = false;
        thread_worker.join();
    }

    void start_thread();

    bool filter(const std::string&);
    void load_filter(const std::string&);

    void compute_serializable(const std::string&, const std::string&);
    bool is_serializable(const std::string&) const;

    // Queue processor (main thread function)
    bool processQueue();

    // Wrapper for an element of the queue
    bool push(const TupleQueueElement&);

    // Store trace
    bool push(const std::string& thread_name, const std::string& class_name, 
              const std::string& method_name, const std::string& signature_name,
              const unsigned long long methodId, const unsigned long long parent_methodId);

    bool pop(const std::string& thread_name, const unsigned long long methodId);


    void set_database(const std::string& db_name) {
        database.update_database_path(db_name);
    }

    void dump();

    unsigned int queue_size() const {
        return queue.size();
    }

    static bool toString(JNIEnv*, jobject, std::string&);


private:
    TraceStore(const TraceStore&) {}
    TraceStore& operator=(const TraceStore&) {
        return *this;
    }
};


#endif
