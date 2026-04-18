CREATE TABLE Units(
	id INTEGER IDENTITY(1,1) PRIMARY KEY,
	imya NVARCHAR(100) NOT NULL	,
	shortName NVARCHAR(10) NOT NULL
);

CREATE TABLE ProductClass(
	id INTEGER IDENTITY(1,1) PRIMARY KEY,
	code NVARCHAR(50) UNIQUE NOT NULL,
	imay NVARCHAR(200) NOT NULL,
	isTerminal BIT NOT NULL,
	baseUnitID INT NULL,
	parentID INTEGER NULL,
	orderIndex INTEGER DEFAULT 0,
	CONSTRAINT FK_ProductClass_Unit
		FOREIGN KEY (baseUnitID)
		REFERENCES Units(id),
	CONSTRAINT FK_ProductClass_Parent
		FOREIGN KEY (parentID)
		REFERENCES ProductClass(id)
);

CREATE TABLE tovar(
	id INTEGER IDENTITY(1,1) PRIMARY KEY,
	imya NVARCHAR(200) NOT NULL,
	articleNumber NVARCHAR(50),
	price DECIMAL(10,2) NOT NULL,
	manufacture NVARCHAR(100),
	productClassID INTEGER NOT NULL,
	CONSTRAINT FK_tovar_Class
		FOREIGN KEY(ProductClassID)
		REFERENCES ProductClass(id)
);

