insert into parametr(name,type,enumID,unitID, maxValue, minValue) values('Диаметр','number', NULL, 1, 10, 200),
('Длина','number', NULL, 1, 10, 500),
('Тип двигателя','enum', 1, NULL, NULL, NULL),
('Тип тормозов','enum', 2, NULL, NULL, NULL);

insert into ProductclassParametr(ProductclassID, parametrID) values(2,3),
(4,1),(4,2),(5,1),(3,4),(6,4),(7,1),(7,4);

insert into ProductParameterValue(productID, parameterID, valueNumber) values(1, 1, 84),
(1, 2, 75),(2, 1, 82),(2, 2, 74);

insert into ProductParameterValue(productID, parameterID, valueNumber) values(3, 1, 60),
(4, 1, 65);
insert into ProductParameterValue(productID, parameterID, valueEnumID) values(5, 4, 3),
(6, 4, 3),(7, 4, 3),(8, 4, 3);
insert into ProductParameterValue(productID, parameterID, valueNumber) values(7, 1, 300),
(8, 1, 280);