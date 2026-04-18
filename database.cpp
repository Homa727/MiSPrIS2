#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QDebug>
#include<QDateTime>
#include<QFile>
#include <QDir>
#include<QVector>
#include "database.h"
Database::Database(QObject *parent) : QObject(parent)
{

}
Database::~Database(){

}
void Database::connectToDatabase() {

    database = QSqlDatabase::addDatabase("QODBC");

    QString connectString = "DRIVER={ODBC Driver 17 for SQL Server};";
    connectString.append("SERVER=localhost;");
    connectString.append("Trusted_Connection=yes;");
    connectString.append("DATABASE=mispris;");

    database.setDatabaseName(connectString);
    if (database.open()) {
        qDebug() << "База данных успешно подключена";
    } else {
        QSqlError err = database.lastError();
        qDebug() << "Ошибка подключения к базе данных"<<err.text();
    }
}
void Database::disconnectDatabase() {
    if (database.isOpen()) {
        database.close();
        qDebug() << "База данных отключена";
    }
}

bool Database::AddProductClass(const ProductClass &cls){
    if (classCodeExists(cls.code)) {
        qDebug() << "Ошибка добавления: код" << cls.code << "уже существует";
            return false;
    }
    if (cls.parentID != 0) {
        QSqlQuery checkParent;
        checkParent.prepare("SELECT isTerminal FROM ProductClass WHERE id = :parentID");
        checkParent.bindValue(":parentID", cls.parentID);
        if (!checkParent.exec() || !checkParent.next()) {
            qDebug() << "Ошибка добавления: родительский класс с id" << cls.parentID << "не найден";
            return false;
        }

        bool isParentTerminal = checkParent.value(0).toBool();
        if (isParentTerminal) {
            qDebug() << "Ошибка добавления: родительский класс является терминальным, нельзя добавлять потомков";
            return false;
        }
    }
    QSqlQuery query;
    query.prepare("INSERT INTO ProductClass(code, imay, isTerminal, baseUnitID, parentID, orderIndex)" "VALUES(:code, :name, :isTerminal, :baseUnitID, :parentID, :orderIndex)");
    query.bindValue(":code", cls.code);
    query.bindValue(":name", cls.name);
    query.bindValue(":isTerminal",cls.isTerminal);
    query.bindValue(":baseUnitID",cls.baseUnitID);
    query.bindValue(":parentID",cls.parentID);
    query.bindValue(":orderIndex",cls.orderindex);
    return query.exec();
}

void Database::addUnit(const QString &name, const QString &shortmane){
    QSqlQuery query;
    query.prepare("INSERT INTO Units(name, shortName)" "VALUES(:name, :shortName)");
    query.bindValue(":name" , name);
    query.bindValue(":shortName" , shortmane);
    query.exec();
}
bool Database::deleteProductClass(int id_for_del){
    QSqlQuery checkChildren;
    checkChildren.prepare("SELECT COUNT(*) FROM ProductClass WHERE parentID = :id");
    checkChildren.bindValue(":id", id_for_del);
    if (!checkChildren.exec() || !checkChildren.next()) {
        qDebug() << "Ошибка при проверке потомков:" << checkChildren.lastError().text();
            return false;
    }

    int childCount = checkChildren.value(0).toInt();
    if (childCount > 0) {
        qDebug() << "Ошибка удаления: у класса есть" << childCount << "потомков. Сначала удалите их.";
        return false;
    }
    QSqlQuery query;
    query.prepare("DELETE FROM ProductClass WHERE id=:id");
    query.bindValue(":id", id_for_del);
    return query.exec();
}

void Database::deleteUnit(int id){
    QSqlQuery query;
    query.prepare("DELETE FROM Units" "WHERE id=:id");
    query.bindValue(":id", id);
    query.exec();
}
bool Database::moveProductClass(int classID, int newParentID){
    QSqlQuery query;
    query.prepare("UPDATE ProductClass SET parentID = :newParentID WHERE id=:classID");
    query.bindValue(":classID", classID);
    query.bindValue(":newParentID", newParentID);
    return query.exec();
}
QVector<ProductClass> Database::getAllParents(int classID)
{
    QVector<ProductClass> result;

    QSqlQuery query;

    query.prepare(
        "WITH ParentTree AS ( "
        "SELECT * FROM ProductClass WHERE id = :id "
        "UNION ALL "
        "SELECT pc.* "
        "FROM ProductClass pc "
        "JOIN ParentTree pt ON pt.parentID = pc.id ) "
        "SELECT * FROM ParentTree WHERE id <> :id");

    query.bindValue(":id", classID);

    query.exec();

    while(query.next())
    {
        ProductClass pc;

        pc.id = query.value("id").toInt();
        pc.name = query.value("imay").toString();

        result.push_back(pc);
    }

    return result;
}
QVector<ProductClass> Database::getTerminalClasses(int parentID){
    QVector<ProductClass> result;

    QSqlQuery query;

    query.prepare(
        "WITH ClassTree AS ( "
        "SELECT * FROM ProductClass WHERE id = :id "
        "UNION ALL "
        "SELECT pc.* "
        "FROM ProductClass pc "
        "JOIN ClassTree ct ON pc.parentID = ct.id ) "
        "SELECT * FROM ClassTree WHERE isTerminal = 1");

    query.bindValue(":id", parentID);

    query.exec();

    while(query.next())
    {
        ProductClass pc;

        pc.id = query.value("id").toInt();
        pc.name = query.value("imay").toString();

        result.push_back(pc);
    }

    return result;
}
bool Database::checkCycle(int classID, int parentID)
{
    QSqlQuery query;

    query.prepare(
        "WITH Tree AS ( "
        "SELECT id,parentID FROM ProductClass WHERE id = :parent "
        "UNION ALL "
        "SELECT pc.id, pc.parentID "
        "FROM ProductClass pc "
        "JOIN Tree t ON pc.id = t.parentID ) "
        "SELECT id FROM Tree WHERE id = :class");

    query.bindValue(":parent", parentID);
    query.bindValue(":class", classID);

    query.exec();

    return query.next();
}
QVector<ProductClass> Database::getAllChild(int parentID)
{
    QVector<ProductClass> result;

    QSqlQuery query;

    query.prepare(
        "WITH ClassTree AS ( "
        "SELECT * FROM ProductClass WHERE id = :id "
        "UNION ALL "
        "SELECT pc.* "
        "FROM ProductClass pc "
        "JOIN ClassTree ct ON pc.parentID = ct.id ) "
        "SELECT * FROM ClassTree WHERE id <> :id");

    query.bindValue(":id", parentID);

    query.exec();

    while(query.next())
    {
        ProductClass pc;

        pc.id = query.value("id").toInt();
        pc.code = query.value("code").toString();
        pc.name = query.value("imay").toString();

        result.push_back(pc);
    }

    return result;
}
bool Database::setBaseUnit(int classID, int unitID)
{
    QSqlQuery query;

    query.prepare(
        "UPDATE ProductClass "
        "SET baseUnitID = :unit "
        "WHERE id = :id");

    query.bindValue(":unit", unitID);
    query.bindValue(":id", classID);

    return query.exec();
}
bool Database::changeOrder(int classID, int newOrder)
{
    QSqlQuery query;

    query.prepare(
        "UPDATE ProductClass "
        "SET orderIndex = :order "
        "WHERE id = :id");

    query.bindValue(":order", newOrder);
    query.bindValue(":id", classID);

    return query.exec();
}
QVector<ProductClass> Database::getAllProductClasses()
{
    QVector<ProductClass> result;

    QSqlQuery query;
    query.prepare("SELECT * FROM ProductClass ORDER BY orderIndex");

    if(!query.exec())
    {
        qDebug() << query.lastError();
        return result;
    }

    while(query.next())
    {
        result.push_back(mapProductsClass(query));
    }

    return result;
}
bool Database::classCodeExists(const QString &code)
{
    QSqlQuery query;

    query.prepare("SELECT id FROM ProductClass WHERE code = :code");
    query.bindValue(":code", code);

    if(!query.exec())
    {
        qDebug() << query.lastError();

    }

    return query.next();
}
QVector<Unit> Database::getAllUnits()
{
    QVector<Unit> result;

    QSqlQuery query("SELECT * FROM Units");

    while(query.next())
    {
        Unit u;

        u.id = query.value("id").toInt();
        u.name = query.value("imya").toString();
        u.shortname = query.value("shortName").toString();

        result.push_back(u);
    }

    return result;
}
ProductClass Database::mapProductsClass(QSqlQuery &query)
{
    ProductClass pc;

    pc.id = query.value("id").toInt();
    pc.code = query.value("code").toString();
    pc.name = query.value("imay").toString();
    pc.isTerminal = query.value("isTerminal").toBool();
    pc.baseUnitID = query.value("baseUnitID").toInt();
    pc.parentID = query.value("parentID").toInt();
    pc.orderindex = query.value("orderIndex").toInt();

    return pc;
}
bool Database::addEnum(const QString &name){
    QSqlQuery query;
    query.prepare("INSERT INTO Enum(imya)" "VALUES(:name)");
    query.bindValue(":name",name);
    return query.exec();
}
bool Database::addEnumValue(const EnumValues &ptr){
    QSqlQuery query;
    query.prepare("INSERT INTO EnumValues(enumID, imya, orderID)" "VALUES(:enumID, :code, :orderID)");
    query.bindValue(":enumID", ptr.enumid);
    query.bindValue(":code",ptr.code);
    query.bindValue(":orderID",ptr.orderIndex);
    return query.exec();
}
bool Database::changeEnumValueOrder(int id, int newOrderIndex){
    QSqlQuery query;
    query.prepare("UPDATE EnumValues SET orderID=:newOrderIndex WHERE id=:id");
    query.bindValue(":newOrderIndex",newOrderIndex);
    query.bindValue(":id",id);
    return query.exec();
}
void Database::deleteEnumValue(int id){
    QSqlQuery query;
    query.prepare("DELETE FROM EnumValues WHERE id=:id");
    query.bindValue(":id",id);
    query.exec();
}
bool Database::updateEnumValue(int id, QString newcode){
    QSqlQuery query;
    query.prepare("UPDATE EnumValues SET code=:newcode WHERE id=:id");
    query.bindValue(":newcode",newcode);
    query.bindValue(":id",id);
    return query.exec();
}
QVector<Enum> Database::getEnums(){
    QVector<Enum> result;
    QSqlQuery query;
    query.prepare("SELECT * FROM Enum");
    while(query.next()){
        Enum e;
        e.id=query.value("id").toInt();
        e.name=query.value("imya").toString();
        result.push_back(e);
    }
    return result;
}
QVector<EnumValues> Database::getEnumValues(int enumID){
    QVector<EnumValues> result;
    QSqlQuery query;
    query.prepare("SELECT * FROM EnumValues WHERE enumID=:enumID");
    query.bindValue(":enumID",enumID);
    while(query.next()){
        EnumValues v;
        v.id=query.value("id").toInt();
        v.enumid=query.value("enumID").toInt();
        v.code=query.value("imya").toString();
        v.orderIndex=query.value("orderID").toInt();
        result.push_back(v);
    }
    return result;
}
QVector<EnumValues> Database::getEnumValueByID(int id){
    QVector<EnumValues> result;
    QSqlQuery query;
    query.prepare("SELECT * FROM EnumValues WHERE id=:id");
    query.bindValue(":id",id);
    while(query.next()){
        EnumValues val;
        val.id=query.value("id").toInt();
        val.enumid=query.value("enumID").toInt();
        val.code=query.value("code").toString();
        val.orderIndex=query.value("orderIndex").toInt();
        result.push_back(val);
    }
    return result;
}
