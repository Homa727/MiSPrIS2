#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QDebug>
#include <QDateTime>
#include <QFile>
#include <QDir>
#include <QVector>
#include <QJsonArray>
#include <QJsonObject>
#include "database.h"

Database::Database(QObject *parent) : QObject(parent) {}
Database::~Database() {}

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
    query.prepare("INSERT INTO ProductClass(code, imay, isTerminal, baseUnitID, parentID, orderIndex) "
                  "VALUES(:code, :name, :isTerminal, :baseUnitID, :parentID, :orderIndex)");
    query.bindValue(":code", cls.code);
    query.bindValue(":name", cls.name);
    query.bindValue(":isTerminal", cls.isTerminal);
    query.bindValue(":baseUnitID", cls.baseUnitID);
    query.bindValue(":parentID", cls.parentID);
    query.bindValue(":orderIndex", cls.orderindex);
    return query.exec();
}

void Database::addUnit(const QString &name, const QString &shortmane){
    QSqlQuery query;
    query.prepare("INSERT INTO Units(imya, shortName) VALUES(:name, :shortName)");
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
    query.prepare("DELETE FROM Units WHERE id=:id");
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
    query.prepare("UPDATE ProductClass SET baseUnitID = :unit WHERE id = :id");
    query.bindValue(":unit", unitID);
    query.bindValue(":id", classID);
    return query.exec();
}

bool Database::changeOrder(int classID, int newOrder)
{
    QSqlQuery query;
    query.prepare("UPDATE ProductClass SET orderIndex = :order WHERE id = :id");
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
    query.prepare("INSERT INTO [Enum] (imya) VALUES(:name)");
    query.bindValue(":name",name);
    return query.exec();
}

bool Database::addEnumValue(const EnumValues &ptr){
    QSqlQuery query;
    query.prepare("INSERT INTO [EnumValues] (enumID, imya, orderID) VALUES(:enumID, :code, :orderID)");
    query.bindValue(":enumID", ptr.enumid);
    query.bindValue(":code",ptr.code);
    query.bindValue(":orderID",ptr.orderIndex);
    return query.exec();
}

bool Database::changeEnumValueOrder(int id, int newOrderIndex){
    QSqlQuery query;
    query.prepare("UPDATE [EnumValues] SET orderID=:newOrderIndex WHERE id=:id");
    query.bindValue(":newOrderIndex",newOrderIndex);
    query.bindValue(":id",id);
    return query.exec();
}

bool Database::deleteEnumValue(int id){
    QSqlQuery query;
    query.prepare("DELETE FROM [EnumValues] WHERE id=:id");
    query.bindValue(":id",id);
    return query.exec();
}

bool Database::updateEnumValue(int id, QString newcode){
    QSqlQuery query;
    query.prepare("UPDATE [EnumValues] SET imya=:newcode WHERE id=:id");
    query.bindValue(":newcode",newcode);
    query.bindValue(":id",id);
    return query.exec();
}

QVector<Enum> Database::getEnums(){
    QVector<Enum> result;
    QSqlQuery query;
    query.prepare("SELECT * FROM dbo.[Enum]");
    query.exec();
    while(query.next()){
        Enum e;
        e.id=query.value("id").toInt();
        e.name=query.value("imya").toString();
        qDebug() << "Fetched:" << e.id << e.name;
        result.push_back(e);
    }
    qDebug() << query.lastError();
    return result;
}

QVector<EnumValues> Database::getEnumValues(int enumID){
    QVector<EnumValues> result;
    QSqlQuery query;
    query.prepare("SELECT * FROM [EnumValues] WHERE enumID=:enumID");
    query.bindValue(":enumID",enumID);
    query.exec();
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
    query.prepare("SELECT * FROM [EnumValues] WHERE id=:id");
    query.bindValue(":id",id);
    query.exec();
    while(query.next()){
        EnumValues val;
        val.id=query.value("id").toInt();
        val.enumid=query.value("enumID").toInt();
        val.code=query.value("imya").toString();
        val.orderIndex=query.value("orderID").toInt();
        result.push_back(val);
    }
    qDebug() << query.lastError();
    return result;
}

bool Database::addParametr(const Parametr &param){
    QSqlQuery query;
    query.prepare("INSERT INTO Parametr (name,type,enumID,unitID,minValue,maxValue) VALUES(:name, :type, :enumID, :unitID, :minValue, :maxValue)");
    query.bindValue(":name",param.name);
    query.bindValue(":type",param.type);
    query.bindValue(":enumID",param.enumID);
    query.bindValue(":unitID",param.unitID);
    query.bindValue(":minValue",param.minValue);
    query.bindValue(":maxValue",param.maxValue);
    return query.exec();
}

bool Database::addParametrToClass(int classID, int parametrID){
    QSqlQuery query;
    query.prepare("INSERT INTO ProductclassParametr (ProductclassID,parametrID) VALUES(:classID, :parametrID)");
    query.bindValue(":classID",classID);
    query.bindValue(":parametrID",parametrID);
    return query.exec();
}

QVector<Parametr> Database::getClassParametr(int classID){
    QVector<Parametr> result;
    QSqlQuery query;
    query.prepare("SELECT p.* FROM parametr p JOIN ProductclassParametr pc ON p.id = pc.parametrID WHERE pc.ProductclassID = :classID ");
    query.bindValue(":classID",classID);
    query.exec();
    while(query.next()){
        Parametr par;
        par.id=query.value("id").toInt();
        par.name=query.value("name").toString();
        par.type=query.value("type").toString();
        par.enumID=query.value("enumID").toInt();
        par.unitID=query.value("unitID").toInt();
        par.minValue=query.value("minValue").toInt();
        par.maxValue=query.value("maxValue").toInt();
        result.push_back(par);
    }
    return result;
}

bool Database::setNumberParametrValue(int productID, int parametrID, double value){
    QSqlQuery query;
    query.prepare("UPDATE ProductParameterValue SET valueNumber=:value WHERE productID=:productID AND parameterID=:parametrID");
    query.bindValue(":productID",productID);
    query.bindValue(":parametrID",parametrID);
    query.bindValue(":value",value);
    return query.exec();
}

bool Database::setEnumParametrValue(int productID, int parametrID, double enumValueID){
    QSqlQuery query;
    query.prepare("UPDATE ProductParameterValue SET valueEnumID=:enumValueID WHERE productID=:productID AND parameterID=:parametrID");
    query.bindValue(":productID",productID);
    query.bindValue(":parametrID",parametrID);
    query.bindValue(":enumValueID",enumValueID);
    return query.exec();
}

QVector<ProductParametrValue> Database::getProductParametrValue(int productID){
    QVector<ProductParametrValue> result;
    QSqlQuery query;
    query.prepare("SELECT ppv.*, p.name as paramName FROM ProductParameterValue ppv "
                  "JOIN Parametr p ON ppv.parameterID = p.id "
                  "WHERE ppv.productID = :productID");
    query.bindValue(":productID", productID);
    if (!query.exec()) {
        qDebug() << query.lastError();
        return result;
    }
    while(query.next()){
        ProductParametrValue pro;
        pro.id = query.value("id").toInt();
        pro.productID = query.value("productID").toInt();
        pro.parameterID = query.value("parameterID").toInt();
        pro.valueNumber = query.value("valueNumber").toDouble();
        pro.EnumValueID = query.value("valueEnumID").toInt();
        pro.paramName = query.value("paramName").toString();
        result.push_back(pro);
    }
    return result;
}

QVector<Product> Database::findProductByNumberParam(int parameterID, double min, double max, int classId)
{
    QVector<Product> result;
    QSqlQuery query;
    QString sql = "SELECT t.* FROM tovar t "
                  "JOIN ProductParameterValue ppv ON t.id = ppv.productID "
                  "WHERE ppv.parameterID = :paramID "
                  "AND ppv.valueNumber BETWEEN :min AND :max";
    if (classId > 0) sql += " AND t.productClassID = :classId";
    query.prepare(sql);
    query.bindValue(":paramID", parameterID);
    query.bindValue(":min", min);
    query.bindValue(":max", max);
    if (classId > 0) query.bindValue(":classId", classId);
    if (!query.exec()) {
        qDebug() << query.lastError();
        return result;
    }
    while(query.next()){
        Product p;
        p.id = query.value("id").toInt();
        p.name = query.value("imya").toString();
        p.articleNumber = query.value("articleNumber").toString();
        p.price = query.value("price").toDouble();
        p.manufacturer = query.value("manufacture").toString();
        p.productclassID = query.value("productClassID").toInt();
        result.push_back(p);
    }
    return result;
}

QVector<Product> Database::findProductByEnumParam(int parameterID, int enumValueID, int classId)
{
    QVector<Product> result;
    QSqlQuery query;
    QString sql = "SELECT t.* FROM tovar t "
                  "JOIN ProductParameterValue ppv ON t.id = ppv.productID "
                  "WHERE ppv.parameterID = :paramID "
                  "AND ppv.valueEnumID = :enumID";
    if (classId > 0) sql += " AND t.productClassID = :classId";
    query.prepare(sql);
    query.bindValue(":paramID", parameterID);
    query.bindValue(":enumID", enumValueID);
    if (classId > 0) query.bindValue(":classId", classId);
    if (!query.exec()) {
        qDebug() << query.lastError();
        return result;
    }
    while(query.next()){
        Product p;
        p.id = query.value("id").toInt();
        p.name = query.value("imya").toString();
        p.articleNumber = query.value("articleNumber").toString();
        p.price = query.value("price").toDouble();
        p.manufacturer = query.value("manufacture").toString();
        p.productclassID = query.value("productClassID").toInt();
        result.push_back(p);
    }
    return result;
}

QVector<Product> Database::findProductByPrice(double min, double max, int classId)
{
    QVector<Product> result;
    QSqlQuery query;
    QString sql = "SELECT * FROM tovar WHERE price BETWEEN :min AND :max";
    if (classId > 0) sql += " AND productClassID = :classId";
    query.prepare(sql);
    query.bindValue(":min", min);
    query.bindValue(":max", max);
    if (classId > 0) query.bindValue(":classId", classId);
    if (!query.exec()) {
        qDebug() << query.lastError();
        return result;
    }
    while(query.next()){
        Product p;
        p.id = query.value("id").toInt();
        p.name = query.value("imya").toString();
        p.articleNumber = query.value("articleNumber").toString();
        p.price = query.value("price").toDouble();
        p.manufacturer = query.value("manufacture").toString();
        p.productclassID = query.value("productClassID").toInt();
        result.push_back(p);
    }
    return result;
}

QVector<Product> Database::multiSearch(int classId, const QJsonArray &conditions)
{
    QVector<Product> result;

    // Если нет условий, возвращаем все товары класса
    if (conditions.isEmpty()) {
        QSqlQuery query;
        query.prepare("SELECT * FROM tovar WHERE productClassID = :classId");
        query.bindValue(":classId", classId);
        if (!query.exec()) {
            qDebug() << query.lastError();
            return result;
        }
        while (query.next()) {
            Product p;
            p.id = query.value("id").toInt();
            p.name = query.value("imya").toString();
            p.articleNumber = query.value("articleNumber").toString();
            p.price = query.value("price").toDouble();
            p.manufacturer = query.value("manufacture").toString();
            p.productclassID = query.value("productClassID").toInt();
            result.push_back(p);
        }
        return result;
    }

    // Строим сложный запрос
    QString sql = "SELECT DISTINCT t.*";
    QStringList joins;
    QStringList whereClauses;
    QVector<int> paramIds; // для возможного расширения
    int joinCounter = 0;

    whereClauses << "t.productClassID = :classId";

    for (const auto &condVal : conditions) {
        QJsonObject cond = condVal.toObject();
        QString type = cond["type"].toString();
        if (type == "price") {
            double minPrice = cond["minPrice"].toDouble();
            double maxPrice = cond["maxPrice"].toDouble();
            whereClauses << QString("t.price BETWEEN :minPrice_%1 AND :maxPrice_%1").arg(joinCounter);
        }
        else if (type == "number") {
            int paramId = cond["paramId"].toInt();
            double minVal = cond["minValue"].toDouble();
            double maxVal = cond["maxValue"].toDouble();
            QString alias = QString("ppv_%1").arg(joinCounter);
            joins << QString("JOIN ProductParameterValue %1 ON t.id = %1.productID AND %1.parameterID = :paramId_%2")
                         .arg(alias).arg(joinCounter);
            whereClauses << QString("%1.valueNumber BETWEEN :minVal_%2 AND :maxVal_%2")
                                .arg(alias).arg(joinCounter);
            paramIds.append(paramId);
        }
        else if (type == "enum") {
            int paramId = cond["paramId"].toInt();
            int enumValId = cond["enumValueId"].toInt();
            QString alias = QString("ppv_%1").arg(joinCounter);
            joins << QString("JOIN ProductParameterValue %1 ON t.id = %1.productID AND %1.parameterID = :paramId_%2")
                         .arg(alias).arg(joinCounter);
            whereClauses << QString("%1.valueEnumID = :enumValId_%2")
                                .arg(alias).arg(joinCounter);
            paramIds.append(paramId);
        }
        joinCounter++;
    }

    sql += " FROM tovar t " + joins.join(" ");
    sql += " WHERE " + whereClauses.join(" AND ");

    QSqlQuery query;
    query.prepare(sql);
    query.bindValue(":classId", classId);

    int bindCounter = 0;
    for (const auto &condVal : conditions) {
        QJsonObject cond = condVal.toObject();
        QString type = cond["type"].toString();
        if (type == "price") {
            query.bindValue(QString(":minPrice_%1").arg(bindCounter), cond["minPrice"].toDouble());
            query.bindValue(QString(":maxPrice_%1").arg(bindCounter), cond["maxPrice"].toDouble());
        }
        else if (type == "number") {
            query.bindValue(QString(":paramId_%1").arg(bindCounter), cond["paramId"].toInt());
            query.bindValue(QString(":minVal_%1").arg(bindCounter), cond["minValue"].toDouble());
            query.bindValue(QString(":maxVal_%1").arg(bindCounter), cond["maxValue"].toDouble());
        }
        else if (type == "enum") {
            query.bindValue(QString(":paramId_%1").arg(bindCounter), cond["paramId"].toInt());
            query.bindValue(QString(":enumValId_%1").arg(bindCounter), cond["enumValueId"].toInt());
        }
        bindCounter++;
    }

    if (!query.exec()) {
        qDebug() << "MultiSearch error:" << query.lastError();
        return result;
    }

    while (query.next()) {
        Product p;
        p.id = query.value("id").toInt();
        p.name = query.value("imya").toString();
        p.articleNumber = query.value("articleNumber").toString();
        p.price = query.value("price").toDouble();
        p.manufacturer = query.value("manufacture").toString();
        p.productclassID = query.value("productClassID").toInt();
        result.push_back(p);
    }
    return result;
}
bool Database::addBOMPosition(int parentID, int componentID, double quantity, double forQuantity, bool isBase, int unitID){
    if (parentID == componentID) {
        qDebug() << "BOM: изделие не может входить само в себя";
        return false;
    }
    if (checkBOMCycle(parentID, componentID)) {
        qDebug() << "BOM: добавление создаст цикл";
        return false;
    }
    QSqlQuery q;
    q.prepare("INSERT INTO BOM(parentID, componentID, quantity, forQuantity, isBase, unitID) "
              "VALUES(:p, :c, :q, :fq, :ib, :u)");
    q.bindValue(":p", parentID);
    q.bindValue(":c", componentID);
    q.bindValue(":q", quantity);
    q.bindValue(":fq", forQuantity);
    q.bindValue(":ib", isBase);
    q.bindValue(":u", unitID);
    if (!q.exec()){
        qDebug() << q.lastError();
        return false;
    }
    return true;
}
bool Database::updateBOMPosition(int bomID, double quantity, double forQuantity, bool isBase){
    QSqlQuery q;
    q.prepare("UPDATE BOM SET quantity=:q, forQuantity=:fq, isBase=:ib WHERE id=:id");
    q.bindValue(":q", quantity);
    q.bindValue(":fq", forQuantity);
    q.bindValue(":ib", isBase);
    q.bindValue(":id", bomID);
    return q.exec();
}
bool Database::deleteBOMPosition(int bomID){
    QSqlQuery q;
    q.prepare("DELETE FROM BOM WHERE id=:id");
    q.bindValue(":id", bomID);
    return q.exec();
}
QVector<BOM> Database::getBOM(int parentID){
    QVector<BOM> result;
    QSqlQuery q;
    q.prepare("SELECT * FROM BOM WHERE parentID = :p ORDER BY id");
    q.bindValue(":p", parentID);
    if (!q.exec()) { qDebug() << q.lastError(); return result; }
    while (q.next()) {
        BOM p;
        p.id          = q.value("id").toInt();
        p.parentID    = q.value("parentID").toInt();
        p.componentID = q.value("componentID").toInt();
        p.quantity    = q.value("quantity").toDouble();
        p.for_quantity = q.value("forQuantity").toDouble();   // ← ваше поле
        p.isBase      = q.value("isBase").toBool();
        p.unitID      = q.value("unitID").toInt();
        p.level       = 1;
        p.cumQuantity = p.quantity / (p.for_quantity > 0 ? p.for_quantity : 1.0);
        result.push_back(p);
    }
    return result;
}
QVector<BOM> Database::findFullBOM(int productID){
    QVector<BOM> result;
    QSqlQuery q;
    q.prepare(
        "WITH BOMTree AS ( "
        "  SELECT b.id, b.parentID, b.componentID, b.quantity, b.forQuantity, "
        "         b.isBase, b.unitID, "
        "         CAST(b.quantity / NULLIF(b.forQuantity,0) AS DECIMAL(18,8)) AS cumQty, "
        "         1 AS lvl "
        "  FROM BOM b WHERE b.parentID = :pid "
        "  UNION ALL "
        "  SELECT b.id, b.parentID, b.componentID, b.quantity, b.forQuantity, "
        "         b.isBase, b.unitID, "
        "         CAST(bt.cumQty * b.quantity / NULLIF(b.forQuantity,0) AS DECIMAL(18,8)), "
        "         bt.lvl + 1 "
        "  FROM BOM b JOIN BOMTree bt ON b.parentID = bt.componentID ) "
        "SELECT * FROM BOMTree ORDER BY lvl, id");
    q.bindValue(":pid", productID);
    if (!q.exec()) { qDebug() << q.lastError(); return result; }
    while (q.next()) {
        BOM p;
        p.id          = q.value("id").toInt();
        p.parentID    = q.value("parentID").toInt();
        p.componentID = q.value("componentID").toInt();
        p.quantity    = q.value("quantity").toDouble();
        p.for_quantity = q.value("forQuantity").toDouble();
        p.isBase      = q.value("isBase").toBool();
        p.unitID      = q.value("unitID").toInt();
        p.level       = q.value("lvl").toInt();
        p.cumQuantity = q.value("cumQty").toDouble();
        result.push_back(p);
    }
    return result;
}

QVector<CalcResult> Database::calcBOM(int productID, double qty){
    QVector<CalcResult> result;
    QSqlQuery q;
    q.prepare(
        "WITH BOMTree AS ( "
        "  SELECT b.componentID, "
        "         CAST(b.quantity / NULLIF(b.forQuantity,0) AS DECIMAL(18,8)) AS cumQty "
        "  FROM BOM b WHERE b.parentID = :pid "
        "  UNION ALL "
        "  SELECT b.componentID, "
        "         CAST(bt.cumQty * b.quantity / NULLIF(b.forQuantity,0) AS DECIMAL(18,8)) "
        "  FROM BOM b JOIN BOMTree bt ON b.parentID = bt.componentID ) "
        "SELECT t.productClassID, pc.imay AS className, u.shortName AS unitName, "
        "       SUM(bt.cumQty) * :qty AS totalQty "
        "FROM BOMTree bt "
        "JOIN tovar t ON t.id = bt.componentID "
        "JOIN ProductClass pc ON pc.id = t.productClassID "
        "LEFT JOIN Units u ON u.id = pc.baseUnitID "
        "WHERE pc.isTerminal = 1 "
        "GROUP BY t.productClassID, pc.imay, u.shortName");
    q.bindValue(":pid", productID);
    q.bindValue(":qty", qty);
    if (!q.exec()) { qDebug() << q.lastError(); return result; }
    while (q.next()) {
        CalcResult r;
        r.materialClassID = q.value("productClassID").toInt();
        r.className       = q.value("className").toString();
        r.unitShortName   = q.value("unitName").toString();
        r.totalQuantity   = q.value("totalQty").toDouble();
        result.push_back(r);
    }
    return result;
}
bool Database::checkBOMCycle(int parentID, int componentID){
    QSqlQuery q;
    q.prepare(
        "WITH Tree AS ( "
        "  SELECT componentID FROM BOM WHERE parentID = :comp "
        "  UNION ALL "
        "  SELECT b.componentID FROM BOM b JOIN Tree t ON b.parentID = t.componentID ) "
        "SELECT 1 FROM Tree WHERE componentID = :parent");
    q.bindValue(":comp", componentID);
    q.bindValue(":parent", parentID);
    if (!q.exec()) return true;   // ошибка → считаем циклом, безопаснее
    return q.next();
}
// ==================== Модификации и изменения ====================

bool Database::createModification(int baseProductID, const QString &name,
                                  const QString &article, double price,
                                  const QString &manufacturer, int &newProductID)
{
    QSqlQuery q;
    q.prepare("SELECT productClassID FROM tovar WHERE id=:id");
    q.bindValue(":id", baseProductID);
    if (!q.exec() || !q.next()) return false;
    int classID = q.value(0).toInt();

    q.prepare("INSERT INTO tovar(imya, articleNumber, price, manufacture, "
              "productClassID, baseProductID) "
              "VALUES(:n, :a, :p, :m, :c, :b)");
    q.bindValue(":n", name);
    q.bindValue(":a", article);
    q.bindValue(":p", price);
    q.bindValue(":m", manufacturer);
    q.bindValue(":c", classID);
    q.bindValue(":b", baseProductID);
    if (!q.exec()) { qDebug() << q.lastError(); return false; }
    newProductID = q.lastInsertId().toInt();

    // копируем базовую спецификацию
    q.prepare("INSERT INTO BOM(parentID, componentID, quantity, forQuantity, isBase, unitID) "
              "SELECT :new, componentID, quantity, forQuantity, 1, unitID "
              "FROM BOM WHERE parentID = :base AND isBase = 1");
    q.bindValue(":new", newProductID);
    q.bindValue(":base", baseProductID);
    q.exec();

    registerChange(newProductID, "MODIFY",
                   QString("Создана модификация на основе изделия %1").arg(baseProductID));
    return true;
}

bool Database::registerChange(int productID, const QString &type,
                              const QString &description)
{
    QSqlQuery q;
    q.prepare("INSERT INTO ProductChange(productID, changeType, changeDescription) "
              "VALUES(:p, :t, :d)");
    q.bindValue(":p", productID);
    q.bindValue(":t", type);
    q.bindValue(":d", description);
    return q.exec();
}

QVector<ProductChange> Database::getProductHistory(int productID)
{
    QVector<ProductChange> result;
    QSqlQuery q;
    q.prepare("SELECT * FROM ProductChange WHERE productID=:p ORDER BY changeDate DESC");
    q.bindValue(":p", productID);
    q.exec();
    while (q.next()) {
        ProductChange c;
        c.id                = q.value("id").toInt();
        c.productID         = q.value("productID").toInt();
        c.changeDate        = q.value("changeDate").toDateTime();  // ← QDateTime, как у вас
        c.changeType        = q.value("changeType").toString();
        c.changeDescription = q.value("changeDescription").toString();
        result.push_back(c);
    }
    return result;
}
