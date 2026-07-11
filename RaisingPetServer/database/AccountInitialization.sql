USE RaisingPet;

SET FOREIGN_KEY_CHECKS = 0;

-- Reset stock-related player state.
TRUNCATE TABLE StockPrice;
TRUNCATE TABLE StockTrade;
TRUNCATE TABLE StockSale;
TRUNCATE TABLE StockHolding;
TRUNCATE TABLE Stock;

-- Reset savings/loan subscriptions while keeping product tables intact.
TRUNCATE TABLE PlayerLoan;
TRUNCATE TABLE PlayerSavings;

SET FOREIGN_KEY_CHECKS = 1;

-- Keep accounts, but reset runtime/player-owned state.
UPDATE Player
SET
    Money = 0,
    IsOnline = FALSE;
