USE RaisingPet;

-- Reset stock-related player state.
DELETE FROM StockPrice;
DELETE FROM StockTrade;
DELETE FROM StockSale;
DELETE FROM StockHolding;
DELETE FROM Stock;

-- Reset savings/loan subscriptions while keeping product tables intact.
DELETE FROM PlayerLoan;
DELETE FROM PlayerSavings;

-- Keep accounts, but reset runtime/player-owned state.
UPDATE Player
SET
    Money = 0,
    IsOnline = FALSE,
    IsActive = FALSE;
