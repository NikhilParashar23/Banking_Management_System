#ifndef DB_H
#define DB_H

#include <mysql/mysql.h>
#include <string>

// Thin wrapper around the MySQL C API.
// Keeps all raw mysql_* calls in one place so the rest of the
// codebase never has to touch MYSQL* directly.
class Database {
public:
    Database(const std::string& host, const std::string& user,
              const std::string& password, const std::string& dbName,
              unsigned int port = 3306);
    ~Database();

    bool connect();
    MYSQL* raw() { return conn; }

    // Runs a query with no result set expected (INSERT/UPDATE/START TRANSACTION...)
    bool execute(const std::string& query);

    void beginTransaction();
    void commit();
    void rollback();

private:
    MYSQL* conn;
    std::string host, user, password, dbName;
    unsigned int port;
};

#endif
