USE RaisingPet;

ALTER TABLE Player
    ADD COLUMN Nickname VARCHAR(32)
    CHARACTER SET utf8mb4 COLLATE utf8mb4_0900_ai_ci NULL
    AFTER Password,
    ADD COLUMN HasLoggedIn BOOLEAN NOT NULL DEFAULT FALSE
    COMMENT 'TRUE after the player finishes first-time nickname setup'
    AFTER Nickname,
    ADD UNIQUE KEY UQ_Player_Nickname (Nickname),
    ADD CONSTRAINT CK_Player_Nickname
        CHECK (Nickname IS NULL OR CHAR_LENGTH(TRIM(Nickname)) > 0);

-- Existing test accounts do not have nickname/first-login consistency.
-- Clear them after child rows so the new flow starts cleanly.
SET FOREIGN_KEY_CHECKS = 0;
TRUNCATE TABLE StockPrice;
TRUNCATE TABLE StockTrade;
TRUNCATE TABLE StockSale;
TRUNCATE TABLE StockHolding;
TRUNCATE TABLE Stock;
TRUNCATE TABLE PlayerLoan;
TRUNCATE TABLE PlayerSavings;
TRUNCATE TABLE Player;
SET FOREIGN_KEY_CHECKS = 1;
