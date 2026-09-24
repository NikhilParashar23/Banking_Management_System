#include "account.h"
#include "crypto_utils.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <vector>

// ---------- helpers ----------

static std::string escape(Database& db, const std::string& s) {
    std::vector<char> buf(s.size() * 2 + 1);
    mysql_real_escape_string(db.raw(), buf.data(), s.c_str(), s.size());
    return std::string(buf.data());
}

bool Account::accountExists(Database& db, int accountNo) {
    std::ostringstream q;
    q << "SELECT account_no FROM accounts WHERE account_no = " << accountNo
      << " AND status = 'ACTIVE'";
    db.execute(q.str());
    MYSQL_RES* res = mysql_store_result(db.raw());
    bool exists = res && mysql_num_rows(res) > 0;
    if (res) mysql_free_result(res);
    return exists;
}

std::string Account::getAccountType(Database& db, int accountNo) {
    std::ostringstream q;
    q << "SELECT account_type FROM accounts WHERE account_no = " << accountNo;
    db.execute(q.str());
    MYSQL_RES* res = mysql_store_result(db.raw());
    std::string type = "SAVINGS";
    if (res) {
        if (MYSQL_ROW row = mysql_fetch_row(res)) type = row[0];
        mysql_free_result(res);
    }
    return type;
}

bool Account::logTransaction(Database& db, int accountNo,
                              const std::string& type, double amount,
                              double balanceAfter, int relatedAcc) {
    std::ostringstream q;
    q << "INSERT INTO transactions (account_no, type, amount, balance_after, related_account_no) VALUES ("
      << accountNo << ", '" << type << "', " << amount << ", " << balanceAfter << ", ";
    if (relatedAcc == -1) q << "NULL"; else q << relatedAcc;
    q << ")";
    return db.execute(q.str());
}

// ---------- public operations ----------

int Account::createAccount(Database& db, const std::string& name,
                            const std::string& pin,
                            const std::string& accountType,
                            double initialDeposit) {
    std::string salt = CryptoUtils::generateSalt();
    std::string hash = CryptoUtils::hashPin(pin, salt);
    std::string safeName = escape(db, name);

    std::ostringstream q;
    q << "INSERT INTO accounts (name, salt, pin_hash, account_type, balance) VALUES ('"
      << safeName << "', '" << salt << "', '" << hash << "', '"
      << accountType << "', " << initialDeposit << ")";

    if (!db.execute(q.str())) return -1;

    int newAccNo = (int)mysql_insert_id(db.raw());
    logTransaction(db, newAccNo, "DEPOSIT", initialDeposit, initialDeposit);
    return newAccNo;
}

bool Account::login(Database& db, int accountNo, const std::string& pin) {
    std::ostringstream q;
    q << "SELECT salt, pin_hash FROM accounts WHERE account_no = " << accountNo
      << " AND status = 'ACTIVE'";
    db.execute(q.str());
    MYSQL_RES* res = mysql_store_result(db.raw());
    if (!res) return false;

    MYSQL_ROW row = mysql_fetch_row(res);
    if (!row) { mysql_free_result(res); return false; }

    std::string salt = row[0];
    std::string storedHash = row[1];
    mysql_free_result(res);

    return CryptoUtils::hashPin(pin, salt) == storedHash;
}

bool Account::deposit(Database& db, int accountNo, double amount) {
    if (amount <= 0) {
        std::cout << "Deposit amount must be positive.\n";
        return false;
    }

    std::ostringstream q;
    q << "UPDATE accounts SET balance = balance + " << amount
      << " WHERE account_no = " << accountNo;
    if (!db.execute(q.str())) return false;

    double newBalance;
    getBalance(db, accountNo, newBalance);
    return logTransaction(db, accountNo, "DEPOSIT", amount, newBalance);
}

bool Account::withdraw(Database& db, int accountNo, double amount) {
    if (amount <= 0) {
        std::cout << "Withdrawal amount must be positive.\n";
        return false;
    }

    double currentBalance;
    if (!getBalance(db, accountNo, currentBalance)) return false;

    std::string type = getAccountType(db, accountNo);
    double minRequired = (type == "SAVINGS") ? MIN_SAVINGS_BALANCE : 0.0;

    if (currentBalance - amount < minRequired) {
        std::cout << "Insufficient balance. Minimum balance of " << minRequired
                  << " must be maintained.\n";
        return false;
    }

    std::ostringstream q;
    q << "UPDATE accounts SET balance = balance - " << amount
      << " WHERE account_no = " << accountNo;
    if (!db.execute(q.str())) return false;

    double newBalance;
    getBalance(db, accountNo, newBalance);
    return logTransaction(db, accountNo, "WITHDRAW", amount, newBalance);
}

bool Account::transfer(Database& db, int fromAcc, int toAcc, double amount) {
    if (amount <= 0) {
        std::cout << "Transfer amount must be positive.\n";
        return false;
    }
    if (fromAcc == toAcc) {
        std::cout << "Cannot transfer to the same account.\n";
        return false;
    }
    if (!accountExists(db, toAcc)) {
        std::cout << "Destination account does not exist.\n";
        return false;
    }

    double fromBalance;
    getBalance(db, fromAcc, fromBalance);
    std::string type = getAccountType(db, fromAcc);
    double minRequired = (type == "SAVINGS") ? MIN_SAVINGS_BALANCE : 0.0;

    if (fromBalance - amount < minRequired) {
        std::cout << "Insufficient balance for transfer.\n";
        return false;
    }

    // ---- This is the core "why it can't half-fail" part ----
    // Everything between beginTransaction() and commit() is applied
    // as a single atomic unit. If any step fails, rollback() undoes
    // ALL of it -- so money can never leave fromAcc without
    // arriving in toAcc.
    mysql_autocommit(db.raw(), 0);
    db.beginTransaction();

    std::ostringstream debit, credit;
    debit << "UPDATE accounts SET balance = balance - " << amount
          << " WHERE account_no = " << fromAcc;
    credit << "UPDATE accounts SET balance = balance + " << amount
           << " WHERE account_no = " << toAcc;

    bool ok = db.execute(debit.str()) && db.execute(credit.str());

    if (ok) {
        double fromNew, toNew;
        getBalance(db, fromAcc, fromNew);
        getBalance(db, toAcc, toNew);
        ok = logTransaction(db, fromAcc, "TRANSFER_OUT", amount, fromNew, toAcc)
          && logTransaction(db, toAcc, "TRANSFER_IN", amount, toNew, fromAcc);
    }

    if (ok) {
        db.commit();
    } else {
        db.rollback();
        std::cout << "Transfer failed, changes rolled back.\n";
    }

    mysql_autocommit(db.raw(), 1);
    return ok;
}

bool Account::getBalance(Database& db, int accountNo, double& outBalance) {
    std::ostringstream q;
    q << "SELECT balance FROM accounts WHERE account_no = " << accountNo;
    db.execute(q.str());
    MYSQL_RES* res = mysql_store_result(db.raw());
    if (!res) return false;

    MYSQL_ROW row = mysql_fetch_row(res);
    if (!row) { mysql_free_result(res); return false; }

    outBalance = std::stod(row[0]);
    mysql_free_result(res);
    return true;
}

void Account::printStatement(Database& db, int accountNo, int lastN) {
    std::ostringstream q;
    q << "SELECT type, amount, balance_after, txn_time FROM transactions "
      << "WHERE account_no = " << accountNo
      << " ORDER BY txn_time DESC LIMIT " << lastN;
    db.execute(q.str());
    MYSQL_RES* res = mysql_store_result(db.raw());
    if (!res) return;

    std::cout << "\n-- Last " << lastN << " transactions --\n";
    std::cout << std::left << std::setw(15) << "Type"
              << std::setw(12) << "Amount"
              << std::setw(15) << "Balance After"
              << "Time\n";

    MYSQL_ROW row;
    while ((row = mysql_fetch_row(res))) {
        std::cout << std::left << std::setw(15) << row[0]
                  << std::setw(12) << row[1]
                  << std::setw(15) << row[2]
                  << row[3] << "\n";
    }
    mysql_free_result(res);
}
