#ifndef DATABASE_H
#define DATABASE_H
#include <QObject>
#include <QSqlQuery>
#include <QSqlError>
#include <QVector>
#include <QMap>
#include <QDebug>

struct Unit{
    int id;
    QString name;
    QString shortname;
};
struct ProductClass{
    int id;
    QString code;
    QString name;
    bool isTerminal;
    int baseUnitID;
    int parentID;
    int orderindex;
};
struct Product{
    int id;
    QString name;
    QString articleNumber;
    double price;
    QString manufacturer;
    int productclassID;
};
struct Enum{
    int id;
    QString name;
};
struct EnumValues{
    int id;
    int enumid;
    QString code;
    int orderIndex;
};


class Database : public QObject
{
    Q_OBJECT

public:
    explicit Database(QObject *parent = nullptr);
    ~Database();

    void connectToDatabase();
    void disconnectDatabase();
    void addUnit( const QString &name, const QString &shortmane);
    void deleteUnit(int id);
    bool setBaseUnit(int classID, int unitID);
    QVector<Unit> getAllUnits();

    bool AddProductClass(const ProductClass &cls);
    bool deleteProductClass(int id_for_del);
    bool moveProductClass(int classID, int newParentID);

    QVector<ProductClass> getAllProductClasses();
    QVector<ProductClass> getAllChild(int parentID);
    QVector<ProductClass> getAllDEscendants(int parentID);
    QVector<ProductClass> getAllParents(int classID);
    QVector<ProductClass> getTerminalClasses(int parentID);

    bool classCodeExists(const QString &code);
    bool checkCycle( int classID, int newParentID);
    bool addEnum(const QString &name);
    bool addEnumValue(const EnumValues &ptr);
    bool updateEnumValue(int id, QString newcode);
    void deleteEnumValue(int id);
    bool changeEnumValueOrder(int id,int newOrderIndex);
    QVector<EnumValues> getEnumValues(int enumID);
    QVector<Enum> getEnums();
    QVector<EnumValues> getEnumValueByID(int id);


    //QVector<Product> getAllProduct();
    //QVector<Product> getProductsByClass(int classID);
    bool changeOrder(int classID,int newOrderIndex);
private:
    QSqlDatabase database;

    ProductClass mapProductsClass(QSqlQuery &query);
    Product  mapProducts(QSqlQuery &query);

};


#endif // DATABASE_H
