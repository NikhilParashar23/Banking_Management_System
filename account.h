#ifndef ACCOUNT_H
#define ACCOUNT_H

#include "db.h"
#include <string>

// Minimum balance a SAVINGS account must keep after any withdrawal.
const double MIN_SAVINGS_BALANCE = 500.00;

class Account {
public:
    // Returns the new account number, or -1 on failure.
    static int createAccount(Database& db, const std::string& name,
                              const std::string& pin,
                              const std::string& accountType,
                              double initialDeposit);

    // Verifies account_no + pin against the stored salt/hash.
    static bool login(Database& db, int accountNo, const std::string& pin);

    static bool deposit(Database& db, int accountNo, double amount);
    static bool withdraw(Database& db, int accountNo, double amount);

    // Atomic transfer: debits fromAcc, credits toAcc.
    // Either both legs succeed and are committed, or neither is applied.
    static bool transfer(Database& db, int fromAcc, int toAcc, double amount);

    static bool getBalance(Database& db, int accountNo, double& outBalance);
    static void printStatement(Database& db, int accountNo, int lastN = 5);

    static bool accountExists(Database& db, int accountNo);
    static std::string getAccountType(Database& db, int accountNo);

private:
    static bool logTransaction(Database& db, int accountNo,
                                const std::string& type, double amount,
                                double balanceAfter, int relatedAcc = -1);
};

#endif
