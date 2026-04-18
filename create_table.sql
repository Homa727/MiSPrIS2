INSERT INTO PRODUCTCLASS(CODE, IMAY, ISTERMINAL, BASEUNITID, PARENTID, ORDERINDEX)
VALUES
('AUTO','АВТОЗАПЧАСТИ',0,NULL,NULL,1);
INSERT INTO PRODUCTCLASS(CODE, IMAY, ISTERMINAL, BASEUNITID, PARENTID, ORDERINDEX)
VALUES
('ENGINE','ДВИГАТЕЛЬ',0,NULL,1,1),
('BRAKE','ТОРМОЗА',0,NULL,1,2);
INSERT INTO PRODUCTCLASS(CODE, IMAY, ISTERMINAL, BASEUNITID, PARENTID, ORDERINDEX)
VALUES
('PISTON','ПОРШНИ',1,1,2,1),
('TURBO','ТУРБИНА',1,1,2,2),
('PADS','ТОРМОЗНЫЕ КОЛОДКИ',1,2,3,1),
('DISK','ТОРМОЗНЫЕ ДИСКИ',1,2,3,2);

INSERT INTO TOVAR(IMYA, ARTICLENUMBER, PRICE, MANUFACTURE, PRODUCTCLASSID)
VALUES
('ПОРШЕНЬ BMW M54','PST-BMW-001',5400.00,'MAHLE',4),
('ПОРШЕНЬ BMW N20','PST-BMW-002',6000.00,'MAHLE',4),
('ТУРБИНА GARRETT GT1749','TRB-4490',30000.00,'GARRETT',5),
('ТУРБИНА HKS 2835','TRB-1182',25000.00,'HKS',5),
('ТОРМОЗНЫЕ КОЛОДКИ BREMBO','BRK-2451',10000.00,'BREMBO',6),
('ТОРМОЗНЫЕ КОЛОДКИ BOSH','BRK-1190',5000.00,'BOSH',6),
('ТОРМОЗНОЙ ДИСК BREMBO','DSC-7011',15000.00,'BREMBO',7),
('ТОРМОЗНОЙ ДИСК BOSH','DSC-1024',10000.00,'BOSH',7);
INSERT INTO UNITS(IMYA, SHORTNAME)
VALUES
('ШТУКА','ШТ'),
('КОМПЛЕКТ','КОМ'),
('ЛИТР','Л'),
('ПАРА','ПАР'),
('НАБОР','НАБ');
INSERT INTO Enum(imya)
VALUES('тип двигателя'),('тип тормозов');
INSERT INTO EnumValues(enumID, imya, orderID)
VALUES(1, 'бензин',1), (1,'дизель',2), (2,'дисковые',1), (2,'барабанные',2);
INSERT INTO ProductClassEnum(ProductClassID,enumID)
VALUES(2,1), (3,2);
