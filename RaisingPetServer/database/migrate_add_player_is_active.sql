USE RaisingPet;

ALTER TABLE Player
    ADD COLUMN IsActive BOOLEAN NOT NULL DEFAULT FALSE
    COMMENT 'Manual activity flag controlled by admin client'
    AFTER IsOnline;
