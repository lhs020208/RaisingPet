-- RaisingPet database schema
-- Target: MySQL 8.0.16+

CREATE DATABASE IF NOT EXISTS `RaisingPet`
    CHARACTER SET utf8mb4
    COLLATE utf8mb4_0900_ai_ci;

USE `RaisingPet`;

CREATE TABLE `Player` (
    `PlayerID` VARCHAR(32) CHARACTER SET ascii COLLATE ascii_bin NOT NULL,
    `Password` VARCHAR(64) CHARACTER SET ascii COLLATE ascii_bin NOT NULL,
    `Nickname` VARCHAR(32) CHARACTER SET utf8mb4 COLLATE utf8mb4_0900_ai_ci NULL,
    `HasLoggedIn` BOOLEAN NOT NULL DEFAULT FALSE
        COMMENT 'TRUE after the player finishes first-time nickname setup',
    `IsOnline` BOOLEAN NOT NULL DEFAULT FALSE,
    `IsActive` BOOLEAN NOT NULL DEFAULT FALSE COMMENT 'Manual activity flag controlled by admin client',
    `Money` BIGINT UNSIGNED NOT NULL DEFAULT 0,
    PRIMARY KEY (`PlayerID`),
    UNIQUE KEY `UQ_Player_Nickname` (`Nickname`),
    CONSTRAINT `CK_Player_PlayerID`
        CHECK (`PlayerID` REGEXP '^[A-Za-z0-9]+$'),
    CONSTRAINT `CK_Player_Password`
        CHECK (`Password` REGEXP '^[A-Za-z0-9]+$'),
    CONSTRAINT `CK_Player_Nickname`
        CHECK (`Nickname` IS NULL OR CHAR_LENGTH(TRIM(`Nickname`)) > 0)
) ENGINE = InnoDB;

CREATE TABLE `InstallmentSavings` (
    `SavingsID` INT UNSIGNED NOT NULL AUTO_INCREMENT,
    `DepositMoney` BIGINT UNSIGNED NOT NULL,
    `ReturnMoney` BIGINT UNSIGNED NOT NULL,
    `Duration` INT UNSIGNED NOT NULL COMMENT 'Duration in seconds',
    PRIMARY KEY (`SavingsID`),
    CONSTRAINT `CK_InstallmentSavings_Money`
        CHECK (`DepositMoney` > 0 AND `ReturnMoney` > `DepositMoney`),
    CONSTRAINT `CK_InstallmentSavings_Duration`
        CHECK (`Duration` > 0)
) ENGINE = InnoDB;

CREATE TABLE `PlayerSavings` (
    `PlayerID` VARCHAR(32) CHARACTER SET ascii COLLATE ascii_bin NOT NULL,
    `SavingsID` INT UNSIGNED NOT NULL,
    `StartTime` DATETIME NOT NULL,
    `EndTime` DATETIME NOT NULL,
    PRIMARY KEY (`PlayerID`),
    KEY `IX_PlayerSavings_SavingsID` (`SavingsID`),
    KEY `IX_PlayerSavings_EndTime` (`EndTime`),
    CONSTRAINT `FK_PlayerSavings_Player`
        FOREIGN KEY (`PlayerID`) REFERENCES `Player` (`PlayerID`)
        ON UPDATE CASCADE ON DELETE CASCADE,
    CONSTRAINT `FK_PlayerSavings_Product`
        FOREIGN KEY (`SavingsID`) REFERENCES `InstallmentSavings` (`SavingsID`)
        ON UPDATE CASCADE ON DELETE RESTRICT,
    CONSTRAINT `CK_PlayerSavings_Time`
        CHECK (`EndTime` > `StartTime`)
) ENGINE = InnoDB;

CREATE TABLE `Loan` (
    `LoanID` INT UNSIGNED NOT NULL AUTO_INCREMENT,
    `BorrowMoney` BIGINT UNSIGNED NOT NULL,
    `RepaymentMoney` BIGINT UNSIGNED NOT NULL,
    `Duration` INT UNSIGNED NOT NULL COMMENT 'Duration in seconds',
    PRIMARY KEY (`LoanID`),
    CONSTRAINT `CK_Loan_Money`
        CHECK (`BorrowMoney` > 0 AND `RepaymentMoney` > `BorrowMoney`),
    CONSTRAINT `CK_Loan_Duration`
        CHECK (`Duration` > 0)
) ENGINE = InnoDB;

CREATE TABLE `PlayerLoan` (
    `PlayerID` VARCHAR(32) CHARACTER SET ascii COLLATE ascii_bin NOT NULL,
    `LoanID` INT UNSIGNED NOT NULL,
    `StartTime` DATETIME NOT NULL,
    `RepaymentTime` DATETIME NOT NULL,
    `IsWaitingForCollection` BOOLEAN NOT NULL DEFAULT FALSE,
    PRIMARY KEY (`PlayerID`),
    KEY `IX_PlayerLoan_LoanID` (`LoanID`),
    KEY `IX_PlayerLoan_RepaymentTime` (`RepaymentTime`),
    CONSTRAINT `FK_PlayerLoan_Player`
        FOREIGN KEY (`PlayerID`) REFERENCES `Player` (`PlayerID`)
        ON UPDATE CASCADE ON DELETE CASCADE,
    CONSTRAINT `FK_PlayerLoan_Product`
        FOREIGN KEY (`LoanID`) REFERENCES `Loan` (`LoanID`)
        ON UPDATE CASCADE ON DELETE RESTRICT,
    CONSTRAINT `CK_PlayerLoan_Time`
        CHECK (`RepaymentTime` > `StartTime`)
) ENGINE = InnoDB;

CREATE TABLE `Stock` (
    `StockID` INT UNSIGNED NOT NULL AUTO_INCREMENT,
    `StockName` VARCHAR(50) CHARACTER SET utf8mb4 COLLATE utf8mb4_0900_ai_ci NOT NULL,
    `IssuerID` VARCHAR(32) CHARACTER SET ascii COLLATE ascii_bin NOT NULL,
    `CurrentPrice` BIGINT UNSIGNED NOT NULL,
    `TotalQuantity` INT UNSIGNED NOT NULL DEFAULT 1000,
    `UnsoldQuantity` INT UNSIGNED NOT NULL DEFAULT 800,
    `PriceUpdatedTime` DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    PRIMARY KEY (`StockID`),
    UNIQUE KEY `UQ_Stock_IssuerID` (`IssuerID`),
    CONSTRAINT `FK_Stock_Issuer`
        FOREIGN KEY (`IssuerID`) REFERENCES `Player` (`PlayerID`)
        ON UPDATE CASCADE ON DELETE RESTRICT,
    CONSTRAINT `CK_Stock_StockName`
        CHECK (CHAR_LENGTH(`StockName`) > 0),
    CONSTRAINT `CK_Stock_CurrentPrice`
        CHECK (`CurrentPrice` >= 1),
    CONSTRAINT `CK_Stock_TotalQuantity`
        CHECK (`TotalQuantity` = 1000),
    CONSTRAINT `CK_Stock_UnsoldQuantity`
        CHECK (`UnsoldQuantity` <= 800)
) ENGINE = InnoDB;

CREATE TABLE `StockHolding` (
    `PlayerID` VARCHAR(32) CHARACTER SET ascii COLLATE ascii_bin NOT NULL,
    `StockID` INT UNSIGNED NOT NULL,
    `Quantity` INT UNSIGNED NOT NULL,
    PRIMARY KEY (`PlayerID`, `StockID`),
    KEY `IX_StockHolding_StockID` (`StockID`),
    CONSTRAINT `FK_StockHolding_Player`
        FOREIGN KEY (`PlayerID`) REFERENCES `Player` (`PlayerID`)
        ON UPDATE CASCADE ON DELETE CASCADE,
    CONSTRAINT `FK_StockHolding_Stock`
        FOREIGN KEY (`StockID`) REFERENCES `Stock` (`StockID`)
        ON UPDATE CASCADE ON DELETE CASCADE,
    CONSTRAINT `CK_StockHolding_Quantity`
        CHECK (`Quantity` > 0)
) ENGINE = InnoDB;

CREATE TABLE `StockSale` (
    `SaleID` BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
    `SellerID` VARCHAR(32) CHARACTER SET ascii COLLATE ascii_bin NOT NULL,
    `StockID` INT UNSIGNED NOT NULL,
    `Quantity` INT UNSIGNED NOT NULL,
    `RegisteredTime` DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    PRIMARY KEY (`SaleID`),
    KEY `IX_StockSale_StockTime` (`StockID`, `RegisteredTime`),
    KEY `IX_StockSale_SellerID` (`SellerID`),
    CONSTRAINT `FK_StockSale_Seller`
        FOREIGN KEY (`SellerID`) REFERENCES `Player` (`PlayerID`)
        ON UPDATE CASCADE ON DELETE CASCADE,
    CONSTRAINT `FK_StockSale_Stock`
        FOREIGN KEY (`StockID`) REFERENCES `Stock` (`StockID`)
        ON UPDATE CASCADE ON DELETE CASCADE,
    CONSTRAINT `CK_StockSale_Quantity`
        CHECK (`Quantity` > 0)
) ENGINE = InnoDB;

CREATE TABLE `StockTrade` (
    `TradeID` BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
    `StockID` INT UNSIGNED NOT NULL,
    `BuyerID` VARCHAR(32) CHARACTER SET ascii COLLATE ascii_bin NOT NULL,
    `SellerID` VARCHAR(32) CHARACTER SET ascii COLLATE ascii_bin NULL
        COMMENT 'NULL when buying unsold issuer stock',
    `Quantity` INT UNSIGNED NOT NULL,
    `Price` BIGINT UNSIGNED NOT NULL COMMENT 'Price per share at trade time',
    `TradeTime` DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    PRIMARY KEY (`TradeID`),
    KEY `IX_StockTrade_StockTime` (`StockID`, `TradeTime`),
    KEY `IX_StockTrade_BuyerID` (`BuyerID`),
    KEY `IX_StockTrade_SellerID` (`SellerID`),
    CONSTRAINT `FK_StockTrade_Stock`
        FOREIGN KEY (`StockID`) REFERENCES `Stock` (`StockID`)
        ON UPDATE CASCADE ON DELETE RESTRICT,
    CONSTRAINT `FK_StockTrade_Buyer`
        FOREIGN KEY (`BuyerID`) REFERENCES `Player` (`PlayerID`)
        ON UPDATE CASCADE ON DELETE RESTRICT,
    CONSTRAINT `FK_StockTrade_Seller`
        FOREIGN KEY (`SellerID`) REFERENCES `Player` (`PlayerID`)
        ON UPDATE CASCADE ON DELETE RESTRICT,
    CONSTRAINT `CK_StockTrade_Quantity`
        CHECK (`Quantity` > 0),
    CONSTRAINT `CK_StockTrade_Price`
        CHECK (`Price` > 0),
    CONSTRAINT `CK_StockTrade_Participants`
        CHECK (`SellerID` IS NULL OR `BuyerID` <> `SellerID`)
) ENGINE = InnoDB;

CREATE TABLE `StockPrice` (
    `StockPriceID` BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
    `StockID` INT UNSIGNED NOT NULL,
    `PreviousPrice` BIGINT UNSIGNED NOT NULL,
    `NewPrice` BIGINT UNSIGNED NOT NULL,
    `BoughtQuantity` INT UNSIGNED NOT NULL DEFAULT 0,
    `SoldQuantity` INT UNSIGNED NOT NULL DEFAULT 0,
    `ChangeReason` VARCHAR(255) CHARACTER SET utf8mb4 COLLATE utf8mb4_0900_ai_ci NULL COMMENT 'Optional display text for price change reason',
    `ChangedTime` DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    PRIMARY KEY (`StockPriceID`),
    KEY `IX_StockPrice_StockTime` (`StockID`, `ChangedTime`),
    CONSTRAINT `FK_StockPrice_Stock`
        FOREIGN KEY (`StockID`) REFERENCES `Stock` (`StockID`)
        ON UPDATE CASCADE ON DELETE CASCADE,
    CONSTRAINT `CK_StockPrice_Prices`
        CHECK (`PreviousPrice` > 0 AND `NewPrice` > 0)
) ENGINE = InnoDB;
