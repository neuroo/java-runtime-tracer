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
#include <string>
#include <iostream>
#include "database.h"

using namespace db;
using namespace std;


Database::Database()
: usable(false), sqlite_db(0) {
    // Instanciate the in-memory SQLite
    if (SQLITE_OK == sqlite3_open(":memory:", &sqlite_db)) {
        backup_path = "java-trace.db";
        usable = true;
        create_schema();
    }
}


unsigned int Database::thread(const string& thread_name) {
    if (!ready())
        return 0;
    if (SQLITE_OK != sqlite3_bind_text(insert_thread, 1,  thread_name.c_str(), thread_name.size(), SQLITE_STATIC)) {
        return 0;
    }
    if (SQLITE_DONE != sqlite3_step(insert_thread)) {
        return 0;
    }
    sqlite3_reset(insert_thread);  
    return static_cast<unsigned int>(sqlite3_last_insert_rowid(sqlite_db));
}

unsigned int Database::method(const string& method_name) {
    if (!ready())
        return 0;
    if (SQLITE_OK != sqlite3_bind_text(insert_method, 1,  method_name.c_str(), method_name.size(), SQLITE_STATIC)) {
        return 0;
    }
    if (SQLITE_DONE != sqlite3_step(insert_method)) {
        return 0;
    }
    sqlite3_reset(insert_method);  
    return static_cast<unsigned int>(sqlite3_last_insert_rowid(sqlite_db));
}

unsigned int Database::clazz(const std::string& class_name) {
    if (!ready())
        return 0;
    if (SQLITE_OK != sqlite3_bind_text(insert_class, 1,  class_name.c_str(), class_name.size(), SQLITE_STATIC)) {
        return 0;
    }
    if (SQLITE_DONE != sqlite3_step(insert_class)) {
        return 0;
    }
    sqlite3_reset(insert_class);  
    return static_cast<unsigned int>(sqlite3_last_insert_rowid(sqlite_db));
}

unsigned int Database::signature(const std::string& signature_name) {
    if (!ready())
        return 0;
    if (SQLITE_OK != sqlite3_bind_text(insert_signature, 1,  signature_name.c_str(), signature_name.size(), SQLITE_STATIC)) {
        return 0;
    }
    if (SQLITE_DONE != sqlite3_step(insert_signature)) {
        return 0;
    }
    sqlite3_reset(insert_signature);  
    return static_cast<unsigned int>(sqlite3_last_insert_rowid(sqlite_db));
}

unsigned int Database::fqn(const unsigned int class_id, const unsigned int method_id, const unsigned int signature_id, const unsigned int jmethod_id) {
    if (!ready())
        return 0;
    if (SQLITE_OK != sqlite3_bind_int(insert_fqn, 1,  class_id)) {
        return 0;
    }
    if (SQLITE_OK != sqlite3_bind_int(insert_fqn, 2,  method_id)) {
        return 0;
    }
    if (SQLITE_OK != sqlite3_bind_int(insert_fqn, 3,  signature_id)) {
        return 0;
    }
    if (SQLITE_OK != sqlite3_bind_int(insert_fqn, 4,  jmethod_id)) {
        return 0;
    }
    if (SQLITE_DONE != sqlite3_step(insert_fqn)) {
        return 0;
    }
    sqlite3_reset(insert_fqn);  
    return static_cast<unsigned int>(sqlite3_last_insert_rowid(sqlite_db));
}

unsigned int Database::trace(const unsigned int thread_id, const unsigned int fqn_id, const unsigned long long  parent_trace_id) {
    if (!ready())
        return 0;
    if (SQLITE_OK != sqlite3_bind_int(insert_trace, 1,  thread_id)) {
        return 0;
    }
    if (SQLITE_OK != sqlite3_bind_int(insert_trace, 2,  fqn_id)) {
        return 0;
    }
    if (SQLITE_OK != sqlite3_bind_int(insert_trace, 3,  parent_trace_id)) {
        return 0;
    }
    if (SQLITE_DONE != sqlite3_step(insert_trace)) {
        return 0;
    }
    sqlite3_reset(insert_trace);  
    return static_cast<unsigned int>(sqlite3_last_insert_rowid(sqlite_db));
}


Database::~Database() {
    sqlite3_finalize(insert_trace);
    sqlite3_finalize(insert_thread);
    sqlite3_finalize(insert_fqn);
    sqlite3_finalize(insert_class);
    sqlite3_finalize(insert_method);
    sqlite3_finalize(insert_signature);

    // Close the DB when everything is cleared
    sqlite3_close(sqlite_db);
}


// Create a timestamp string to use in the name of the dumped DB
string Database::trace_time() const {
    time_t t = time(0);
    struct tm *local = localtime(&t);
    char output[30];
    strftime(output, 30, "%m-%d-%H-%M-%S", local);
    return string(output);
}


// Create the DB schema
void Database::create_schema() {
    if (sqlite_db && usable) {
        sqlite3_exec(sqlite_db, db_schema, 0, 0, 0);
        
        // Instanciate the prepared statements
        sqlite3_prepare(sqlite_db, stmt_insert_trace, -1, &insert_trace, 0);
        sqlite3_prepare(sqlite_db, stmt_insert_thread, -1, &insert_thread, 0);
        sqlite3_prepare(sqlite_db, stmt_insert_fqn, -1, &insert_fqn, 0);
        sqlite3_prepare(sqlite_db, stmt_insert_class, -1, &insert_class, 0);
        sqlite3_prepare(sqlite_db, stmt_insert_method, -1, &insert_method, 0);
        sqlite3_prepare(sqlite_db, stmt_insert_signature, -1, &insert_signature, 0);
    }
}

// Stolen from SQLite documentation
void Database::backup(bool isSave) {
    int rc;                   /* Function return code */
    sqlite3 *pFile;           /* Database connection opened on zFilename */
    sqlite3_backup *pBackup;  /* Backup object used to copy data */
    sqlite3 *pTo;             /* Database to copy to (pFile or pInMemory) */
    sqlite3 *pFrom;           /* Database to copy from (pFile or pInMemory) */

    rc = sqlite3_open(backup_path.c_str(), &pFile);
    if( rc==SQLITE_OK ){
        pFrom = (isSave ? sqlite_db : pFile);
        pTo   = (isSave ? pFile     : sqlite_db);

        pBackup = sqlite3_backup_init(pTo, "main", pFrom, "main");
        if( pBackup ){
          (void)sqlite3_backup_step(pBackup, -1);
          (void)sqlite3_backup_finish(pBackup);
        }
        rc = sqlite3_errcode(pTo);
    }

    sqlite3_close(pFile);
}


