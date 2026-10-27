ALTER TABLE tovar ADD baseProductID INT NULL
    CONSTRAINT FK_tovar_Base FOREIGN KEY (baseProductID) REFERENCES tovar(id);