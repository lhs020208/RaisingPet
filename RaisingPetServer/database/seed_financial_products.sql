-- RaisingPet financial product seed data
-- Duration is stored in seconds.

USE `RaisingPet`;

INSERT INTO `InstallmentSavings`
    (`SavingsID`, `DepositMoney`, `ReturnMoney`, `Duration`)
VALUES
    (1, 150, 240, 600),
    (2, 500, 850, 1800),
    (3, 1500, 2700, 3600),
    (4, 5000, 9500, 7200),
    (5, 15000, 30000, 14400),
    (6, 50000, 105000, 21600),
    (7, 150000, 330000, 28800),
    (8, 500000, 1150000, 43200),
    (9, 2000000, 4800000, 64800),
    (10, 10000000, 25000000, 86400)
ON DUPLICATE KEY UPDATE
    `DepositMoney` = VALUES(`DepositMoney`),
    `ReturnMoney` = VALUES(`ReturnMoney`),
    `Duration` = VALUES(`Duration`);

INSERT INTO `Loan`
    (`LoanID`, `BorrowMoney`, `RepaymentMoney`, `Duration`)
VALUES
    (1, 200, 210, 600),
    (2, 700, 740, 1800),
    (3, 2000, 2120, 3600),
    (4, 7000, 7450, 7200),
    (5, 20000, 21400, 14400),
    (6, 70000, 75000, 21600),
    (7, 200000, 216000, 28800),
    (8, 700000, 760000, 43200),
    (9, 2500000, 2750000, 64800),
    (10, 12000000, 13500000, 86400)
ON DUPLICATE KEY UPDATE
    `BorrowMoney` = VALUES(`BorrowMoney`),
    `RepaymentMoney` = VALUES(`RepaymentMoney`),
    `Duration` = VALUES(`Duration`);
