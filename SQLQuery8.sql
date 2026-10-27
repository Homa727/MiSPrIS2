CREATE TABLE BOM(
id INT IDENTITY(1,1) not NULL,
parentID int not NULL,
componentID int not NULL,
quantity DECIMAL(18,4) not NULL,
for_quantity DECIMAL(18,4) not NULL,
isBase BIT DEFAUlT 1,
unitID INT NULL
CONSTRAINT FK_BOM_parent FOREIGN KEY (parentID) REFERENCES tovar(id),
CONSTRAINT FK_BOM_component FOREIGN KEY (componentID) REFERENCES tovar(id),
CONSTRAINT FK_BOM_unit FOREIGN KEY (unitID) REFERENCES units(id),
CONSTRAINT CHK_BOM_NoSelf CHECK (parentID <> componentID)
);