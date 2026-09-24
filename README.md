# Terminal-Based Banking Management System

C++ console application backed by MySQL. No frontend — pure terminal UI,
matching a C++ / MySQL / DSA background.

## Features
- Create Account (Savings / Current) with a salted + SHA-256-hashed PIN
- Login (account number + PIN)
- Deposit
- Withdraw (enforces minimum balance of ₹500 for Savings accounts)
- Transfer between accounts — **atomic**: uses `START TRANSACTION` /
  `COMMIT` / `ROLLBACK` so a transfer can never debit one account
  without crediting the other
- Balance inquiry
- Mini statement (last 5 transactions)

## Files
| File | Purpose |
|---|---|
| `schema.sql` | Creates the database and both tables |
| `db.h` / `db.cpp` | MySQL connection wrapper |
| `crypto_utils.h` / `crypto_utils.cpp` | Salt generation + SHA-256 PIN hashing |
| `account.h` / `account.cpp` | All banking logic (create, deposit, withdraw, transfer, statement) |
| `main.cpp` | Terminal menu loop |

## Setup

### 1. Install dependencies (Ubuntu/Debian)
```bash
sudo apt-get install default-libmysqlclient-dev libssl-dev mysql-server
```

### 2. Create the database
```bash
sudo mysql -u root -p < schema.sql
```

### 3. Set your credentials
Open `main.cpp` and update this line with your actual MySQL username/password:
```cpp
Database db("127.0.0.1", "root", "your_mysql_password", "bank_system");
```

### 4. Compile
```bash
g++ -std=c++17 main.cpp db.cpp account.cpp crypto_utils.cpp -o bank_app \
    $(mysql_config --cflags --libs) -lssl -lcrypto
```

### 5. Run
```bash
./bank_app
```

## Key design points to mention in your interview

1. **Password security**: PINs are never stored in plain text. Each account
   gets a random salt; the stored hash is `SHA256(salt + pin)`. Salting
   means two users with the same PIN get completely different hashes,
   which defeats precomputed rainbow-table attacks.

2. **Transaction atomicity (transfer)**: `Account::transfer()` wraps the
   debit, credit, and both transaction-log inserts in a single MySQL
   transaction. If any step fails, `rollback()` undoes everything already
   done in that transaction — so the system can never end up in a state
   where money left one account but never reached the other. This is the
   ACID property of **Atomicity** in action.

3. **Data integrity via schema**: `transactions.account_no` has a foreign
   key into `accounts`, so you cannot log a transaction against an account
   that doesn't exist. `balance_after` is stored per transaction so a
   statement can be printed without recomputing running totals.

4. **Business rule enforcement in the app layer**: minimum balance for
   Savings accounts is checked in C++ before any withdrawal or transfer
   is attempted, so invalid operations never even reach the database.

## Possible extensions if you have time
- Admin menu (view all accounts, freeze/unfreeze an account)
- Simple fraud-flagging: log a transaction to a `flagged_transactions`
  table if the amount exceeds a threshold
- Interest calculation for Savings accounts
