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
#include "config.h"
#include "store.h"

#include <iostream>
#include <exception>
#include <string>
#include <vector>
#include <list>
#include <map>
#include <fstream>

using namespace std;
using boost::tuple;


void TraceStore::start_thread() {
    thread_worker = boost::thread(boost::bind(&TraceStore::processQueue, this));
}

bool TraceStore::processQueue() {
#ifdef DEBUG_INLINE
    cout << "TraceStore::processQueue- Start to process the queue..." << endl;
#endif

    while (running) {
        try {
            TupleQueueElement item;
            if (queue.try_pop(item)) {

                #ifdef DEBUG_INLINE         
                    cout << "TraceStore::processQueue- Popped element form the queue, store it in the DB" << endl;
                #endif
                push(item);
            }

        }
        catch (std::exception& e) {
#ifdef DEBUG_INLINE
            cout << "TraceStore::processQueue- Exception=" << e.what() << endl;
#endif          
            return false;
        }
    }

    if (!queue.empty()) {
        while(!queue.empty()) {
            TupleQueueElement item;
            if (queue.try_pop(item)) {
                push(item);
            }
        }
    }

    return true;
}


bool TraceStore::filter(const string& class_name) {
    map<string, bool>::const_iterator filter_iter = filter_cache.find(class_name);
    if (filter_iter != filter_cache.end())
        return filter_iter->second;

    for (list<string>::const_iterator iter=class_whitelist.begin(); iter!=class_whitelist.end(); ++iter) {
        if (class_name.find(*iter) != string::npos) {
            filter_cache[class_name] = false;
            return false;
        }
    }   

    for (list<string>::const_iterator iter=class_blacklist.begin(); iter!=class_blacklist.end(); ++iter) {
        if (class_name.find(*iter) != string::npos) {
            filter_cache[class_name] = true;
            return true;
        }
    }

    filter_cache[class_name] = false;
    return false;
}


void TraceStore::load_filter(const string& filters_filename) {
    ifstream in(filters_filename.c_str());

    cout << "Loading ... " << filters_filename << endl;
    if (!in) {
        cout << "Cannot open the filters at: " << filters_filename << endl;
        return;
    }
    char content[256] = "";
    while(in) {
        in.getline(content, 255);
        string str(content + sizeof(char));

        if (str.size() < 1)
            continue;

        if (content[0] == '+') {
            if (find(class_whitelist.begin(), class_whitelist.end(), str) == class_whitelist.end()) {
                class_whitelist.push_back(str);
            }   
        }
        else {
            if (find(class_blacklist.begin(), class_blacklist.end(), str) == class_blacklist.end()) {
                class_blacklist.push_back(str);
            }   
        }
    }
}


void TraceStore::compute_serializable(const string& clazz, const string& generic) {
    // search for Ljava/io/Serializable
    map<string, bool>::const_iterator serial_iter = serializable.find(clazz);
    if (serial_iter == serializable.end()) {
        serializable[clazz] = generic.find("Ljava/io/Serializable") != string::npos;
    }
}


bool TraceStore::is_serializable(const string& clazz) const {
    map<string, bool>::const_iterator serial_iter = serializable.find(clazz);
    if (serial_iter != serializable.end())
        return serial_iter->second;
    return false;
}

bool TraceStore::pop(const string& thread_name, const unsigned long long methodId) {
    return true;
}


bool TraceStore::push(const string& thread_name, const string& class_name, const string& method_name, const string& signature_name, const unsigned long long methodId, const unsigned long long parent_methodId) {

    TupleQueueElement e(thread_name, class_name, method_name, signature_name, methodId, parent_methodId);
    queue.push(e);
    return true;
}

bool TraceStore::push(const TupleQueueElement& item) {
    unsigned int thread_id = 0, class_id = 0, method_id = 0, signature_id = 0, fqn_id = 0;

    string thread_name = item.get<0>(), 
           class_name = item.get<1>(), 
           method_name = item.get<2>(), 
           signature_name = item.get<3>();

    unsigned long long methodId = item.get<4>(), 
                       parent_methodId = item.get<5>();


    KeyCache::const_iterator cache_iter = thread_cache.find(thread_name);
    if (cache_iter == thread_cache.end()) {
        unsigned int last_thread = database.thread(thread_name);
        thread_cache[thread_name] = last_thread;
        thread_id = last_thread;
    }
    else
        thread_id = cache_iter->second;


    cache_iter = class_cache.find(class_name);
    if (cache_iter == class_cache.end()) {
        unsigned int last_class = database.clazz(class_name);
        class_cache[class_name] = last_class;
        class_id = last_class;
    }
    else
        class_id = cache_iter->second;

    cache_iter = method_cache.find(method_name);
    if (cache_iter == method_cache.end()) {
        unsigned int last_method = database.method(method_name);
        method_cache[method_name] = last_method;
        method_id = last_method;
    }
    else
        method_id = cache_iter->second;


    cache_iter = signature_cache.find(signature_name);
    if (cache_iter == signature_cache.end()) {
        unsigned int last_signature = database.signature(signature_name);
        signature_cache[signature_name] = last_signature;
        signature_id = last_signature;
    }
    else
        signature_id = cache_iter->second;

    TupleFQN tupleFQN(class_id, method_id, signature_id);
    TupleKeyCache::const_iterator tuple_iter = fqn_cache.find(tupleFQN);
    if (tuple_iter == fqn_cache.end()) {
        unsigned int last_fqn = database.fqn(class_id, method_id, signature_id, (unsigned long long)methodId);
        fqn_cache[tupleFQN] = last_fqn;
        fqn_id = last_fqn;
    }
    else
        fqn_id = tuple_iter->second;

    trace_id = database.trace(thread_id, fqn_id, (unsigned long long)parent_methodId);

#ifdef DATABASE_PERIODIC_DUMP
    if (0 == (trace_id % 1000000)) {
        database.save();
    }
#endif

    return true;
}


// Hacky...
void TraceStore::dump() {
    database.save();
}


// utilities to work with java objects
bool TraceStore::toString(JNIEnv* env, jobject obj, string& output) {
    if (!obj)
        return false;

    jobject obj_locref = env->NewGlobalRef(obj);

    cout << "1" << endl;

    jclass locObjectClass = (jclass)env->NewLocalRef(env->GetObjectClass(obj_locref));
    if (!locObjectClass)
        return false;

    if (env->ExceptionOccurred()) {
        cout << "TraceStore::toString- Exception occured" << endl;
    }

    cout << "2" << endl;


    jclass objectClass = env->FindClass("java/lang/Object");
    if (!objectClass)
        return false;

    if (env->ExceptionOccurred()) {
        cout << "TraceStore::toString- Exception occured" << endl;
    }


    jmethodID toString = env->GetMethodID(objectClass, "toString", "()Ljava/lang/String;");
    if (!toString)
        return false;

    if (env->ExceptionOccurred()) {
        cout << "TraceStore::toString- Exception occured" << endl;
    }


    cout << "Here..." << endl;

    jstring javaStr = static_cast<jstring>(env->CallObjectMethod(obj, toString));
    if (!javaStr)
        return false;
    const char* strBuf = env->GetStringUTFChars(javaStr, 0);
    output.assign(strBuf);

    env->ReleaseStringUTFChars(javaStr, strBuf);
    env->DeleteLocalRef(javaStr);
    env->DeleteLocalRef(objectClass);
    env->DeleteLocalRef(locObjectClass);

    return true;
}





























