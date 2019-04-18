#ifndef URLQUERYPARSER_H
#define URLQUERYPARSER_H

#include <functional>
#include <QtCore/QString>
#include "coordinate.h"

class UrlQueryParser
{
public:
    typedef std::function<void(const QString &key, const QString &value)> KeyValueCallback;
    typedef std::function<void(const Coordinate &coordinate)> CoordinateCallback;
    static void parse(const QString &query, const KeyValueCallback &keyValue, const CoordinateCallback &coordinate);
};

#endif // URLQUERYPARSER_H