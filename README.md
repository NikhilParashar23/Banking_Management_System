# 🏦 Terminal-Based Banking Management System

A console-based banking application built in **C++** with a **MySQL** backend. No web framework, no GUI — just a menu-driven terminal program that models core banking operations with a real focus on data integrity and secure credential storage.

## Features

- **Account Creation** — Savings or Current accounts, with a minimum opening balance rule for Savings
- **Secure Login** — PIN is never stored in plain text; each account has a random salt, and only `SHA-256(salt + pin)` is stored
- **Deposit / Withdraw** — enforces a minimum balance on Savings accounts
- **Fund Transfer** — atomic, using MySQL's `START TRANSACTION` / `COMMIT` / `ROLLBACK`, so a transfer can never debit one account without crediting the other
- **Balance Inquiry**
- **Mini Statement** — last 5 transactions for an account

## Tech Stack

| Layer | Technology |
|---|---|
| Language | C++17 |
| Database | MySQL 8 |
| DB Connectivity | MySQL C API (`libmysqlclient`) |
| Security | OpenSSL (SHA-256 hashing) |
| Interface | Terminal / Console |

## Why These Design Choices

**Salted password hashing.** Storing a raw PIN is a liability — anyone with database access could read it directly. Instead, each account gets a random salt at creation time, and the database stores `SHA256(salt + pin)`. Two accounts with the identical PIN end up with completely different stored hashes, which defeats precomputed rainbow-table attacks.

**Atomic transfers.** `Account::transfer()` wraps the debit, the credit, and both transaction-log inserts inside a single MySQL transaction. If any one step fails, `rollback()` undoes everything already applied in that transaction, so the system can never land in a state where money left one account but never reached the other. This is the **Atomicity** property from ACID, applied directly rather than just described.

**Business rules enforced before hitting the DB.** Minimum-balance checks happen in C++ before any withdrawal or transfer query is even sent, so invalid operations never touch the database layer.

## Project Structure

```
banking_system/
├── main.cpp              # Terminal menu loop, program entry point
├── db.h / db.cpp         # MySQL connection wrapper (connect, query, transaction control)
├── account.h / account.cpp   # Core banking logic: create, login, deposit, withdraw, transfer, statement
├── crypto_utils.h / crypto_utils.cpp  # Salt generation + SHA-256 PIN hashing
├── schema.sql             # Database and table definitions
└── README.md
```

## Database Schema

**`accounts`**
| Column | Type | Notes |
|---|---|---|
| account_no | INT, PK, auto-increment | |
| name | VARCHAR(100) | |
| salt | VARCHAR(32) | random per-account salt |
| pin_hash | VARCHAR(64) | SHA-256 hex digest of salt + pin |
| account_type | ENUM('SAVINGS','CURRENT') | |
| balance | DECIMAL(15,2) | |
| status | ENUM('ACTIVE','INACTIVE') | |
| created_at | TIMESTAMP | |

**`transactions`**
| Column | Type | Notes |
|---|---|---|
| txn_id | INT, PK, auto-increment | |
| account_no | INT, FK → accounts | |
| type | ENUM('DEPOSIT','WITHDRAW','TRANSFER_OUT','TRANSFER_IN') | |
| amount | DECIMAL(15,2) | |
| balance_after | DECIMAL(15,2) | avoids recomputation when printing a statement |
| related_account_no | INT, nullable | set only for transfers |
| txn_time | TIMESTAMP | |

## Setup & Installation

### 1. Install dependencies
```bash
sudo apt-get update
sudo apt-get install mysql-server default-libmysqlclient-dev libssl-dev build-essential
```

### 2. Start MySQL
```bash
sudo service mysql start
```

### 3. Create the database
```bash
mysql -u root -p < schema.sql
```

### 4. Configure your credentials
Open `main.cpp` and update this line with your MySQL username/password:
```cpp
Database db("127.0.0.1", "root", "your_mysql_password", "bank_system");
```

### 5. Compile
```bash
g++ -std=c++17 main.cpp db.cpp account.cpp crypto_utils.cpp -o bank_app \
    $(mysql_config --cflags --libs) -lssl -lcrypto
```

### 6. Run
```bash
./bank_app
```

## Sample Usage

```
===== BANKING MANAGEMENT SYSTEM =====
1. Create Account
2. Login
3. Exit
Choose an option: 1

Enter name: Aisha Khan
Set a PIN (4-6 digits): 4821
Account type (1 = SAVINGS, 2 = CURRENT): 1
Initial deposit: 1000
Account created successfully! Your account number is: 1
```

## Possible Future Improvements

- Admin panel to view all accounts and freeze/unfreeze one
- Rule-based fraud flagging (e.g. large or rapid transactions logged to a `flagged_transactions` table)
- Interest calculation for Savings accounts
- Unit tests for balance and transfer edge cases

## License

This project is open source and available under the [MIT License](LICENSE).
