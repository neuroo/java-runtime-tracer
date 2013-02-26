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
#include <iostream>
#include <vector>
#include <string>
#include <stack>

#include <sys/stat.h>
#include <jvmti.h>

#include <boost/tokenizer.hpp>
#include <boost/algorithm/string.hpp>

#include "config.h"
#include "store.h"
using namespace std;


typedef std::stack<jmethodID> MethodStack;
typedef std::map<jthread, MethodStack* > StackThreadMethod;
typedef boost::tokenizer<boost::char_separator<char> > Tokenizer;


static boost::char_separator<char> options_separator(",");

static jvmtiEnv *globalJVMTIInterface = 0;
static jrawMonitorID monitor_lock;

static map<string, string> conf;
static TraceStore store;
static vector<string> loadedClasses;
static StackThreadMethod thread_methodid_stack;



static jvmtiIterationControl JNICALL heapObject(jlong class_tag, jlong size, jlong* tag_ptr, void* user_data) {
    return JVMTI_ITERATION_CONTINUE;
}

// Get the list of loaded classes
// at the init of the VM
static void JNICALL vm_init(jvmtiEnv *jvmti, JNIEnv *env, jthread thread) {
    jint returnCode;
    jint numberOfClasses = 0;
    jclass *classes = 0;

    returnCode =  globalJVMTIInterface->GetLoadedClasses(&numberOfClasses, &classes);
    if (returnCode != JVMTI_ERROR_NONE) {
        cout << "Cannot get the list of classes..." << endl;
        exit(-1);
    }

    for (unsigned i=0; i<numberOfClasses; i++) {
        char* signature = 0;
        char* generic = 0;

        // jvmti->IterateOverHeap(JVMTI_HEAP_OBJECT_EITHER, &heapObject, 0);

        globalJVMTIInterface->GetClassSignature(classes[i], &signature, &generic);

        if(signature) {
            returnCode = globalJVMTIInterface->Deallocate((unsigned char*) signature);
        }
        loadedClasses.push_back(string(signature));
    }
    cout << "Agent::vm_init- Loaded classes: " << loadedClasses.size() << endl;
}


static void JNICALL vm_death(jvmtiEnv *jvmti, JNIEnv *env) {
    // Need a failsafe here?
    cout << "Agent::vm_death- Does this happen?" << endl;
}


static void JNICALL vm_exception(jvmtiEnv *jvmti_env, JNIEnv* env, jthread thr, jmethodID method, jlocation location, jobject exception, jmethodID catch_method, jlocation catch_location) {
    cout << "Agent::vm_exception- Does this happen?" << endl;
}



static void get_classname(jvmtiEnv *jvmti, jmethodID methodId, string& clazz, string& generic) {
    jclass declaring_class;
    char *class_signature, *class_generic;

    // Get the Class information
    jvmti->GetMethodDeclaringClass(methodId, &declaring_class);
    jvmti->GetClassSignature(declaring_class, &class_signature, &class_generic);
    
    if (class_signature) {
        clazz.assign(class_signature);
        jvmti->Deallocate(reinterpret_cast<unsigned char*>(class_signature));
    }
    if (class_generic) {
        generic.assign(class_generic);
        jvmti->Deallocate(reinterpret_cast<unsigned char*>(class_generic));
    }
}

static void get_metod(jvmtiEnv *jvmti, jmethodID methodId, string& method, string& sig) {
    char *method_name, *method_signature, *method_generic;

    // Get the method
    jvmti->GetMethodName(methodId, &method_name, &method_signature, &method_generic);

    if (method_name) {
        method.assign(method_name);
        jvmti->Deallocate(reinterpret_cast<unsigned char*>(method_name));
    }
    if (method_signature) {
        sig.assign(method_signature);
        jvmti->Deallocate(reinterpret_cast<unsigned char*>(method_signature));
    }
    if (method_generic) {
        jvmti->Deallocate(reinterpret_cast<unsigned char*>(method_generic));
    }

}

static void get_thread(jvmtiEnv *jvmti, jthread thread, string& thread_name) {
    // Extract thread-info (its name)
    jvmtiThreadInfo thread_info;
    jvmti->GetThreadInfo(thread, &thread_info);

    thread_name.assign(thread_info.name);
    jvmti->Deallocate(reinterpret_cast<unsigned char*>(thread_info.name));  
}


static void JNICALL method_exit(jvmtiEnv *jvmti, JNIEnv* jni_env, jthread thread, jmethodID methodId, jboolean exception_raised, jvalue return_value) {
    globalJVMTIInterface->RawMonitorEnter(monitor_lock);

    MethodStack *current_stack = 0;
    StackThreadMethod::const_iterator iter = thread_methodid_stack.find(thread);

    if (iter == thread_methodid_stack.end()) {
        thread_methodid_stack[thread] = new MethodStack();
        current_stack = thread_methodid_stack.at(thread);
    }
    else
        current_stack = iter->second;

    current_stack->pop();

    globalJVMTIInterface->RawMonitorExit(monitor_lock);
}

// Dump information for each entry of method (at each call)
// TODO: cache the methodId -> tuple(class, method, signature)
static void JNICALL method_entry(jvmtiEnv *jvmti, JNIEnv* jni_env, jthread thread, jmethodID methodId) {
    globalJVMTIInterface->RawMonitorEnter(monitor_lock);
    MethodStack *current_stack = 0;
    StackThreadMethod::const_iterator iter = thread_methodid_stack.find(thread);

    if (iter == thread_methodid_stack.end()) {
        thread_methodid_stack[thread] = new MethodStack();
        current_stack = thread_methodid_stack.at(thread);
    }
    else
        current_stack = iter->second;

    current_stack->push(methodId);


    if (!store.start_recording) {
        struct stat fileInfo;
        bool file_exists = stat("/Users/rgaucher/Downloads/Threadfix/start-recording", &fileInfo) == 0;

        if (file_exists)
            store.start_recording = true;
    }
    else {
        struct stat fileInfo;
        bool file_exists = stat("/Users/rgaucher/Downloads/Threadfix/stop-recording", &fileInfo) == 0;
        if (file_exists) {
            store.start_recording = false;
            store.dump();
        }
    }

    if (!store.start_recording) {
        globalJVMTIInterface->RawMonitorExit(monitor_lock);
        return;
    }

    // Get name of method
    string sig, method, clazz, generic_class, thread_name;

    get_classname(jvmti, methodId, clazz, generic_class);

    // Should we filter out the current class?
    if (store.filter(clazz)) {
        globalJVMTIInterface->RawMonitorExit(monitor_lock);
        return;
    }

    // Capture if the object is serializable
    store.compute_serializable(clazz, generic_class);

    get_metod(jvmti, methodId, method, sig);
    get_thread(jvmti, thread, thread_name);

    store.push(thread_name, clazz, method, sig, 
               reinterpret_cast<unsigned long long>(methodId), 
               reinterpret_cast<unsigned long long>(current_stack->top()));
    /*
    // Get the parameters
    jint size;
    jvmti->GetArgumentsSize(methodId, &size);

    jint entry_count;
    jvmtiLocalVariableEntry* table;
    jvmtiError jerr = jvmti->GetLocalVariableTable(methodId, &entry_count, &table);

    // Are we in code with debugging information? If so, this is
    // our application, and we can go fancy on what we extract
    // from it
    if (jerr == JVMTI_ERROR_NONE) {
        cout << "Agent::method_entry- Thread=" << thread_name << " | Method=" << clazz << method << sig << endl;

        for (unsigned int i=0; i<entry_count; i++) {
            string signature(table[i].signature);

            cout << "\tAgent::method_entry- Variable=" << table[i].name << " Type=" << signature << endl;;
            // Get the content if this is an Object

            jobject value_ptr;
            jvmti->GetLocalObject(thread, (jint)0, table[i].slot, &value_ptr);
            switch(signature[0]) {
                case 'L' : {
                    // For now, we can only get String objects
                    if (signature.find("java/lang/String") != string::npos) {
                        const char *str = {0};
                        str = jni_env->GetStringUTFChars((jstring)value_ptr, 0);
                        cout << "\tAgent::method_entry- Content=" << str << endl;
                        jni_env->ReleaseStringUTFChars((jstring)value_ptr, str);
                    }
                    break;
                }
                default:
                    break;
            }
        }
        jvmti->Deallocate(reinterpret_cast<unsigned char*>(table));
    }
    */
    // Release the lock
    globalJVMTIInterface->RawMonitorExit(monitor_lock);
}





// Agent start method
// Create the capabilities, and associate the callbacks for VM_INIT, and METHOD_ENTRY
JNIEXPORT jint JNICALL Agent_OnLoad(JavaVM *jvm, char *options, void *reserved) {
    cout << "Agent::Agent_OnLoad- start" << endl;
    store.start_thread();
    if (options) {
        // Tokenize the options
        string opts(options);
        Tokenizer list_options(opts, options_separator);
        for (Tokenizer::const_iterator iter=list_options.begin(); iter!=list_options.end(); ++iter) {
            const string& current_option = *iter;
            vector<string> option_splitted;
            boost::split(option_splitted, current_option, boost::is_any_of("="));

            conf[option_splitted[0]] = option_splitted[1];
        }

        // Load the filter
        if (conf.find("filters") != conf.end())
            store.load_filter(conf.at("filters"));

        // Give the database location
        if (conf.find("database") != conf.end())
            store.set_database(conf.at("database"));

    }

    jint returnCode = jvm->GetEnv((void **)&globalJVMTIInterface, JVMTI_VERSION_1_1);
    
    if (returnCode != JNI_OK) {
        return JVMTI_ERROR_UNSUPPORTED_VERSION;
    }

    jvmtiEventCallbacks eventCallbacks;
    jvmtiCapabilities capabilities;
    memset(&capabilities, 0, sizeof(capabilities));


    capabilities.can_generate_method_exit_events = 1;
    capabilities.can_generate_method_entry_events = 1;

    capabilities.can_get_owned_monitor_info = 1;
    capabilities.can_access_local_variables = 1;
    capabilities.can_get_line_numbers = 1;
    capabilities.can_get_source_file_name = 1;
    capabilities.can_signal_thread = 1;
    capabilities.can_tag_objects = 1;

    // Add our capabilities to the env
    globalJVMTIInterface->AddCapabilities(&capabilities);


    memset(&eventCallbacks, 0, sizeof(eventCallbacks));
    eventCallbacks.VMInit = &vm_init;
    eventCallbacks.MethodEntry = &method_entry;
    eventCallbacks.MethodExit = &method_exit;
    eventCallbacks.VMDeath = &vm_death;
    eventCallbacks.Exception = &vm_exception;

    globalJVMTIInterface->SetEventCallbacks(&eventCallbacks, (jint)sizeof(eventCallbacks));
    globalJVMTIInterface->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_VM_INIT, (jthread)0);
    globalJVMTIInterface->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_VM_DEATH, (jthread)0);
    globalJVMTIInterface->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_METHOD_ENTRY, (jthread)0);
    globalJVMTIInterface->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_METHOD_EXIT, (jthread)0);
    globalJVMTIInterface->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_EXCEPTION, (jthread)0);

    globalJVMTIInterface->CreateRawMonitor("agent data", &monitor_lock);

    return JVMTI_ERROR_NONE;
}


JNIEXPORT void JNICALL Agent_OnUnload(JavaVM *jvm) {
    cout << "Agent::Agent_OnUnload- End of the tracing, dumping the database..." << endl;
    cout << "Agent::Agent_OnUnload- Number of messages remaining for processing " << store.queue_size() << endl;

    // Make sure we dump everything...
    store.wait_threads();
    store.dump();

    // Clean our stacks
    for (StackThreadMethod::iterator iter=thread_methodid_stack.begin(); iter!=thread_methodid_stack.end(); ++iter) {
        delete iter->second;
    }

}
