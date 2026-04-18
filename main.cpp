#include <QCoreApplication>
#include <QTcpServer>
#include <QTcpSocket>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>
#include <QUrlQuery>
#include <QMap>
#include "database.h"

Database *g_db = nullptr;

QJsonObject productClassToJson(const ProductClass &pc) {
    QJsonObject obj;
    obj["id"] = pc.id;
    obj["code"] = pc.code;
    obj["name"] = pc.name;
    obj["isTerminal"] = pc.isTerminal;
    obj["baseUnitID"] = pc.baseUnitID;
    obj["parentID"] = pc.parentID;
    obj["orderIndex"] = pc.orderindex;
    return obj;
}

QJsonObject unitToJson(const Unit &u) {
    QJsonObject obj;
    obj["id"] = u.id;
    obj["name"] = u.name;
    obj["shortName"] = u.shortname;
    return obj;
}
QJsonObject EnumToJson(const Enum &E){
    QJsonObject obj;
    obj["id"]=E.id;
    obj["name"]=E.name;
    return obj;
}
QJsonObject EnumValuesToJson( const EnumValues &val){
    QJsonObject obj;
    obj["id"]=val.id;
    obj["enumid"]=val.enumid;
    obj["code"]=val.code;
    obj["orderIndex"]=val.orderIndex;
    return obj;
}

void sendHttpResponse(QTcpSocket *socket, int statusCode, const QString &statusText,
                      const QString &contentType, const QByteArray &body) {
    QString response = QString("HTTP/1.1 %1 %2\r\n"
                               "Content-Type: %3\r\n"
                               "Content-Length: %4\r\n"
                               "Connection: close\r\n"
                               "\r\n")
                           .arg(statusCode)
                           .arg(statusText)
                           .arg(contentType)
                           .arg(body.size());
    socket->write(response.toUtf8() + body);
    socket->flush();
    socket->disconnectFromHost();
}

void sendJsonResponse(QTcpSocket *socket, const QJsonDocument &doc, int status = 200) {
    sendHttpResponse(socket, status, (status == 200) ? "OK" : "Error",
                     "application/json", doc.toJson());
}

void sendErrorResponse(QTcpSocket *socket, const QString &message, int status = 400) {
    QJsonObject obj;
    obj["error"] = message;
    sendJsonResponse(socket, QJsonDocument(obj), status);
}

struct HttpRequest {
    QString method;
    QString path;
    QMap<QString, QString> queryParams;
    QByteArray body;
};

HttpRequest parseHttpRequest(const QByteArray &data) {
    HttpRequest req;
    QList<QByteArray> lines = data.split('\r');
    if (lines.isEmpty()) return req;
    QByteArray firstLine = lines[0];
    QList<QByteArray> parts = firstLine.split(' ');
    if (parts.size() >= 2) {
        req.method = QString::fromUtf8(parts[0]);
        QString fullPath = QString::fromUtf8(parts[1]);
        if (fullPath.contains('?')) {
            QStringList pathParts = fullPath.split('?');
            req.path = pathParts[0];
            QUrlQuery query(pathParts[1]);
            for (const auto &pair : query.queryItems())
                req.queryParams[pair.first] = pair.second;
        } else {
            req.path = fullPath;
        }
    }
    int bodyStart = data.indexOf("\r\n\r\n");
    if (bodyStart != -1)
        req.body = data.mid(bodyStart + 4);
    return req;
}


bool parseJsonBody(const QByteArray &body, QJsonObject &obj, QString &errorMsg) {
    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(body, &err);
    if (err.error != QJsonParseError::NoError) {
        errorMsg = "Invalid JSON: " + QString(err.errorString());
        return false;
    }
    if (!doc.isObject()) {
        errorMsg = "JSON must be an object";
        return false;
    }
    obj = doc.object();
    return true;
}

void handleRequest(QTcpSocket *socket, const HttpRequest &req) {

    if (req.method == "GET" && req.path == "/api/classes") {
        QVector<ProductClass> classes = g_db->getAllProductClasses();
        QJsonArray arr;
        for (const auto &c : classes) arr.append(productClassToJson(c));
        sendJsonResponse(socket, QJsonDocument(arr));
        return;
    }


    if (req.method == "POST" && req.path == "/api/classes") {
        QJsonObject obj;
        QString errMsg;
        if (!parseJsonBody(req.body, obj, errMsg)) {
            sendErrorResponse(socket, errMsg);
            return;
        }
        ProductClass cls;
        cls.code = obj["code"].toString();
        cls.name = obj["name"].toString();
        cls.isTerminal = obj["isTerminal"].toBool();
        cls.baseUnitID = obj["baseUnitID"].toInt();
        cls.parentID = obj["parentID"].toInt();
        cls.orderindex = obj["orderIndex"].toInt();

        if (g_db->AddProductClass(cls))
            sendJsonResponse(socket, QJsonDocument(QJsonObject{{"status", "ok"}}));
        else
            sendErrorResponse(socket, "Failed to add class (code exists or parent terminal)", 500);
        return;
    }


    if (req.method == "PUT" && req.path == "/api/classes/move") {
        QJsonObject obj;
        QString errMsg;
        if (!parseJsonBody(req.body, obj, errMsg)) {
            sendErrorResponse(socket, errMsg);
            return;
        }
        int classID = obj["classID"].toInt();
        int newParentID = obj["newParentID"].toInt();
        if (g_db->moveProductClass(classID, newParentID))
            sendJsonResponse(socket, QJsonDocument(QJsonObject{{"status", "moved"}}));
        else
            sendErrorResponse(socket, "Move failed", 500);
        return;
    }


    if (req.method == "PUT" && req.path == "/api/classes/order") {
        QJsonObject obj;
        QString errMsg;
        if (!parseJsonBody(req.body, obj, errMsg)) {
            sendErrorResponse(socket, errMsg);
            return;
        }
        int classID = obj["classID"].toInt();
        int newOrder = obj["orderIndex"].toInt();
        if (g_db->changeOrder(classID, newOrder))
            sendJsonResponse(socket, QJsonDocument(QJsonObject{{"status", "order changed"}}));
        else
            sendErrorResponse(socket, "Change order failed", 500);
        return;
    }

    if (req.method == "DELETE" && req.path == "/api/classes") {
        if (!req.queryParams.contains("id")) {
            sendErrorResponse(socket, "Missing id parameter");
            return;
        }
        int id = req.queryParams["id"].toInt();
        if (g_db->deleteProductClass(id))
            sendJsonResponse(socket, QJsonDocument(QJsonObject{{"status", "deleted"}}));
        else
            sendErrorResponse(socket, "Delete failed (has children?)", 500);
        return;
    }


    if (req.method == "PUT" && req.path == "/api/classes/baseunit") {
        QJsonObject obj;
        QString errMsg;
        if (!parseJsonBody(req.body, obj, errMsg)) {
            sendErrorResponse(socket, errMsg);
            return;
        }
        int classID = obj["classID"].toInt();
        int unitID = obj["unitID"].toInt();
        if (g_db->setBaseUnit(classID, unitID))
            sendJsonResponse(socket, QJsonDocument(QJsonObject{{"status", "base unit set"}}));
        else
            sendErrorResponse(socket, "Set base unit failed", 500);
        return;
    }

    if (req.method == "GET" && req.path == "/api/classes/checkcode") {
        if (!req.queryParams.contains("code")) {
            sendErrorResponse(socket, "Missing code parameter");
            return;
        }
        QString code = req.queryParams["code"];
        bool exists = g_db->classCodeExists(code);
        QJsonObject resp;
        resp["exists"] = exists;
        sendJsonResponse(socket, QJsonDocument(resp));
        return;
    }


    if (req.method == "GET" && req.path == "/api/classes/checkcycle") {
        if (!req.queryParams.contains("classID") || !req.queryParams.contains("parentID")) {
            sendErrorResponse(socket, "Missing classID or parentID");
            return;
        }
        int classID = req.queryParams["classID"].toInt();
        int parentID = req.queryParams["parentID"].toInt();
        bool cycle = g_db->checkCycle(classID, parentID);
        QJsonObject resp;
        resp["cycle"] = cycle;
        sendJsonResponse(socket, QJsonDocument(resp));
        return;
    }


    if (req.method == "GET" && req.path == "/api/classes/child") {
        if (!req.queryParams.contains("id")) {
            sendErrorResponse(socket, "Missing id parameter");
            return;
        }
        int parentID = req.queryParams["id"].toInt();
        QVector<ProductClass> children = g_db->getAllChild(parentID);
        QJsonArray arr;
        for (const auto &c : children) arr.append(productClassToJson(c));
        sendJsonResponse(socket, QJsonDocument(arr));
        return;
    }


    if (req.method == "GET" && req.path == "/api/classes/parent") {
        if (!req.queryParams.contains("id")) {
            sendErrorResponse(socket, "Missing id parameter");
            return;
        }
        int classID = req.queryParams["id"].toInt();
        QVector<ProductClass> parents = g_db->getAllParents(classID);
        QJsonArray arr;
        for (const auto &c : parents) arr.append(productClassToJson(c));
        sendJsonResponse(socket, QJsonDocument(arr));
        return;
    }


    if (req.method == "GET" && req.path == "/api/classes/terminal") {
        if (!req.queryParams.contains("id")) {
            sendErrorResponse(socket, "Missing id parameter");
            return;
        }
        int parentID = req.queryParams["id"].toInt();
        QVector<ProductClass> terminals = g_db->getTerminalClasses(parentID);
        QJsonArray arr;
        for (const auto &c : terminals) arr.append(productClassToJson(c));
        sendJsonResponse(socket, QJsonDocument(arr));
        return;
    }
    if(req.method == "GET" && req.path == "/api/classes/"){
        QVector<Enum> Enums = g_db->getEnums();
        QJsonArray arr;
        for (const auto &c : Enums) arr.append(EnumToJson(c));
        sendJsonResponse(socket, QJsonDocument(arr));
        return;
    }
    if(req.method == "GET" && req.path == "/api/classes/values"){
        if(!req.queryParams.contains("enumID")){
            sendErrorResponse(socket, "Missing enumID parameter");
            return;
        }
        int enumID = req.queryParams["enumID"].toInt();
        QJsonArray arr;
        QVector<EnumValues> val = g_db->getEnumValues(enumID);
        for(const auto &c: val) arr.append(EnumValuesToJson(c));
        sendJsonResponse(socket, QJsonDocument(arr));
        return;
    }
    if(req.method == "GET" && req.path == "/api/classes/valuesbyid"){
        if(!req.queryParams.contains("id")){
            sendErrorResponse(socket, "Missing id parameter");
            return;
        }
        int id = req.queryParams["id"].toInt();
        QJsonArray arr;
        QVector<EnumValues> val = g_db->getEnumValueByID(id);
        for(const auto &c: val) arr.append(EnumValuesToJson(c));
        sendJsonResponse(socket, QJsonDocument(arr));
        return;
    }
    if(req.method == "POST" && req.path == "/api/classes/"){
        QJsonObject obj;
        QString err;
        if(!parseJsonBody(req.body,obj,err)){
            sendErrorResponse(socket,err);
        }
        QString name=obj["name"];
        if(g_db->addEnum(name)){
            sendJsonResponse(socket, QJsonDocument(QJsonObject{{"status","Enum add"}}));
        }else{
            sendErrorResponse(socket,"Enum don`t add",500);
        }
        return;
    }
    if(req.method == "POST" && req.path == "/api/classes/value"){
        QJsonObject obj;
        QString err;
        if(!parseJsonBody(req.body,obj,err)){
            sendErrorResponse(socket,err);
        }
        EnumValues val;
        val.id=obj["id"].toInt();
        val.enumid=obj["enumID"].toInt();
        val.code=obj["code"];
        val.orderIndex=obj["orderIndex"].toInt();
        if(g_db->addEnumValue(val)){
            sendJsonResponse(socket, QJsonDocument(QJsonObject{{"status","EnumValue add"}}));
        }else{
            sendErrorResponse(socket,"EnumValue don`t add",500);
        }
        return;
    }
    if(req.method == "DELETE" && req.path == "/api/classes/"){
        if(!req.queryParams.contains("id")){
            sendErrorResponse(socket, "Missing id parameter");
            return;
        }
        int id = req.queryParams["id"].toInt();
        if (g_db->deleteProductClass(id))
            sendJsonResponse(socket, QJsonDocument(QJsonObject{{"status", "deleted"}}));
        else
            sendErrorResponse(socket, "Delete failed (has children?)", 500);
        return;
    }
    if(req.method == "PUT" && req.path == "/api/classes/update"){
        QJsonObject obj;
        QString err;
        if(!parseJsonBody(req.body, obj, err)){
            sendErrorResponse(socket, err);
        }
        QString newcode= obj["name"];
        int id= obj["id"].toInt();
        if(g_db->updateEnumValue(id,newcode)){
            sendJsonResponse(socket, QJsonDocument(QJsonObject{{"status"," Code update"}}));
        }else{
            sendErrorResponse(socket, "Code don`t update", 500);
        }
        return;
    }
    if(req.method == "PUT" && req.path == "/api/classes/change"){
        QJsonObject obj;
        QString err;
        if(!parseJsonBody(req.body, obj, err)){
            sendErrorResponse(socket,err);
        }
        int id=obj["id"].toInt();
        int newOrderIndex=obj["orderIndex"].toInt();
        if(g_db->changeEnumValueOrder(id,newOrderIndex)){
            sendJsonResponse(socket, QJsonDocument(QJsonObject{{"status","order is change"}});
        }else{
            sendErrorResponse(socket, "order don`t change", 500);
        }
        return;
    }

    sendErrorResponse(socket, "Not found", 404);
}


int main(int argc, char *argv[]) {
    QCoreApplication a(argc, argv);

    g_db = new Database();
    g_db->connectToDatabase();
    if (!g_db->isOpen()) {
        qCritical() << "Не удалось подключиться к БД";
        return -1;
    }
    qDebug() << "База данных подключена";

    QTcpServer server;
    if (!server.listen(QHostAddress::Any, 8080)) {
        qCritical() << "Не удалось запустить сервер на порту 8080";
        return -1;
    }
    qDebug() << "Сервер запущен на порту 8080";
    qDebug() << "Доступные эндпоинты:";
    qDebug() << "  GET    /api/classes";
    qDebug() << "  POST   /api/classes";
    qDebug() << "  PUT    /api/classes/move";
    qDebug() << "  PUT    /api/classes/order";
    qDebug() << "  DELETE /api/classes?id=...";
    qDebug() << "  PUT    /api/classes/baseunit";
    qDebug() << "  GET    /api/classes/checkcode?code=...";
    qDebug() << "  GET    /api/classes/checkcycle?classID=...&parentID=...";
    qDebug() << "  GET    /api/classes/child?id=...";
    qDebug() << "  GET    /api/classes/parent?id=...";
    qDebug() << "  GET    /api/classes/terminal?id=...";
    qDebug() << "  GET    /api/units";
    qDebug() << "  POST   /api/units";
    qDebug() << "  DELETE /api/units?id=...";

    QObject::connect(&server, &QTcpServer::newConnection, [&]() {
        QTcpSocket *socket = server.nextPendingConnection();
        QObject::connect(socket, &QTcpSocket::readyRead, [socket]() {
            QByteArray data = socket->readAll();
            HttpRequest req = parseHttpRequest(data);
            handleRequest(socket, req);
        });
        QObject::connect(socket, &QTcpSocket::disconnected, socket, &QTcpSocket::deleteLater);
    });

    return a.exec();
}
