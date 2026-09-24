-- ============================================
-- Terminal-Based Banking Management System
-- Database Schema
-- ============================================

CREATE DATABASE IF NOT EXISTS bank_system;
USE bank_system;

-- Stores each customer's account.
-- PIN is never stored in plain text: we store a random salt
-- and the SHA-256 hash of (salt + pin).
CREATE TABLE IF NOT EXISTS accounts (
    account_no      INT AUTO_INCREMENT PRIMARY KEY,
    name            VARCHAR(100)  NOT NULL,
    salt            VARCHAR(32)   NOT NULL,
    pin_hash        VARCHAR(64)   NOT NULL,   -- SHA-256 hex digest = 64 chars
    account_type    ENUM('SAVINGS', 'CURRENT') NOT NULL DEFAULT 'SAVINGS',
    balance         DECIMAL(15,2) NOT NULL DEFAULT 0.00,
    status          ENUM('ACTIVE', 'INACTIVE') NOT NULL DEFAULT 'ACTIVE',
    created_at      TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

-- Every deposit / withdraw / transfer leg is logged here.
-- balance_after lets you print a statement without recomputing history.
CREATE TABLE IF NOT EXISTS transactions (
    txn_id              INT AUTO_INCREMENT PRIMARY KEY,
    account_no          INT NOT NULL,
    type                ENUM('DEPOSIT', 'WITHDRAW', 'TRANSFER_OUT', 'TRANSFER_IN') NOT NULL,
    amount              DECIMAL(15,2) NOT NULL,
    balance_after       DECIMAL(15,2) NOT NULL,
    related_account_no  INT DEFAULT NULL,     -- filled only for transfers
    txn_time            TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (account_no) REFERENCES accounts(account_no)
);

-- Index to make "find account by number" and statements fast.
CREATE INDEX idx_txn_account ON transactions(account_no);
