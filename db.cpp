#include "db.h"
#include <iostream>

Database::Database(const std::string& host, const std::string& user,
                     const std::string& password, const std::string& dbName,
                     unsigned int port)
    : conn(nullptr), host(host), user(user), password(password),
      dbName(dbName), port(port) {}

Database::~Database() {
    if (conn) mysql_close(conn);
}

bool Database::connect() {
    conn = mysql_init(nullptr);
    if (!conn) {
        std::cerr << "mysql_init() failed\n";
        return false;
    }

    if (!mysql_real_connect(conn, host.c_str(), user.c_str(),
                             password.c_str(), dbName.c_str(),
                             port, nullptr, 0)) {
        std::cerr << "Connection failed: " << mysql_error(conn) << "\n";
        return false;
    }

    // autocommit ON by default; transfer() explicitly turns it off
    // for the duration of a transfer, then re-enables it.
    mysql_autocommit(conn, 1);
    return true;
}

bool Database::execute(const std::string& query) {
    if (mysql_query(conn, query.c_str())) {
        std::cerr << "Query failed: " << mysql_error(conn) << "\n";
        return false;
    }
    return true;
}

void Database::beginTransaction() {
    execute("START TRANSACTION");
}

void Database::commit() {
    mysql_commit(conn);
}

void Database::rollback() {
    mysql_rollback(conn);
}
