#include "urlqueryparser.h"
#include <QtCore/QDebug>

void UrlQueryParser::parse(const QString &query, const KeyValueCallback &keyValue, const CoordinateCallback &coordinate)
{
    for (const QString &item : query.split(QLatin1Char('|'))) {
        int colon = item.indexOf(QLatin1Char(':'));
        if (colon > -1) {
            keyValue(item.left(colon), item.mid(colon + 1));
        } else if (item.contains(QLatin1Char(','))) {
            coordinate(Coordinate(item));
        } else {
            qWarning() << item;
        }
    }
}
