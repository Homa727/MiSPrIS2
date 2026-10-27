CREATE TABLE ProductChange (
    id INT IDENTITY(1,1) PRIMARY KEY,
    productID INT NOT NULL,      
    changeDate DATETIME DEFAULT GETDATE(),
    changeDescription NVARCHAR(1000),
    changeType NVARCHAR(50),        
    CONSTRAINT FK_Change_Product FOREIGN KEY (productID) REFERENCES tovar(id)
);