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
#ifndef __DATABASE_H
#define __DATABASE_H

#include <map>
#include <string>
#include <ctime>
#include "config.h"
#include "sqlite3.h"


namespace db {

static const char db_schema[] = "CREATE TABLE IF NOT EXISTS traces (id INTEGER PRIMARY KEY, thread_id INTEGER, fqn_id INTEGER, parent_trace_id INTEGER);\
CREATE TABLE IF NOT EXISTS threads (id INTEGER PRIMARY KEY, thread_name TEXT);\
CREATE TABLE IF NOT EXISTS fqns (id INTEGER PRIMARY KEY, class_id INTEGER, method_id INTEGER, signature_id INTEGER, jmethod_id INTEGER);\
CREATE TABLE IF NOT EXISTS classes (id INTEGER PRIMARY KEY, class_name TEXT);\
CREATE TABLE IF NOT EXISTS methods (id INTEGER PRIMARY KEY, method_name TEXT);\
CREATE TABLE IF NOT EXISTS signatures (id INTEGER PRIMARY KEY, signature_name TEXT);\
DELETE FROM traces; DELETE FROM threads; DELETE FROM fqns; DELETE FROM classes; DELETE FROM methods; DELETE FROM signatures;";

static const char stmt_insert_trace[] = "INSERT INTO traces VALUES (NULL, ?, ?, ?);";
static const char stmt_insert_thread[] = "INSERT INTO threads VALUES (NULL, ?);";
static const char stmt_insert_fqn[] = "INSERT INTO fqns VALUES (NULL, ?, ?, ?, ?);";
static const char stmt_insert_class[] = "INSERT INTO classes VALUES (NULL, ?);";
static const char stmt_insert_method[] = "INSERT INTO methods VALUES (NULL, ?);";
static const char stmt_insert_signature[] = "INSERT INTO signatures VALUES (NULL, ?);";


class Database {
    std::string backup_path;
    bool usable;

    // SQLite objects
    sqlite3* sqlite_db;

    sqlite3_stmt* insert_trace;
    sqlite3_stmt* insert_thread;
    sqlite3_stmt* insert_fqn;
    sqlite3_stmt* insert_class;
    sqlite3_stmt* insert_method;
    sqlite3_stmt* insert_signature;

  private:
    void create_schema();

    std::string trace_time() const;
    
    void backup(bool isSave=true);

    Database(const Database& _p) {}
    Database& operator=(const Database& _p) {
        return *this;
    }

  public:
    Database();
    ~Database();

    inline void update_database_path(const std::string& db_name) {
        backup_path = db_name;
    }

    inline void save() {
        backup();
    }

    // Can we use the database? If not, it's okay.. we'll just
    // don't persist anything!
    inline bool ready() const {
        return usable;
    }

    unsigned int thread(const std::string& thread_name);
    unsigned int method(const std::string& method_name);
    unsigned int clazz(const std::string& class_name);
    unsigned int signature(const std::string& signature_name);
    unsigned int fqn(const unsigned int class_id, const unsigned int method_id, const unsigned int signature_id, const unsigned int jmethod_id);
    unsigned int trace(const unsigned int thread_id, const unsigned int fqn_id, const unsigned long long parent_trace_id=0);

};

}

#endif
