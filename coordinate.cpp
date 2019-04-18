#include "coordinate.h"
#include <QtCore/QDebugStateSaver>

class Coordinate::Private : public QSharedData
{
public:
    Private();
    Private(double latitude, double longitude);
    Private(const QString &latlng);
    Private(const Private &other);
    double latitude;
    double longitude;
};

Coordinate::Private::Private()
    : QSharedData()
    , latitude(0.0)
    , longitude(0.0)
{}

Coordinate::Private::Private(double latitude, double longitude)
    : QSharedData()
    , latitude(latitude)
    , longitude(longitude)
{}

Coordinate::Private::Private(const QString &latlng)
    : QSharedData()
    , latitude(latlng.section(QLatin1Char(','), 0, 0).toDouble())
    , longitude(latlng.section(QLatin1Char(','), 1, 1).toDouble())
{}

Coordinate::Private::Private(const Private &other)
    : QSharedData(other)
    , latitude(other.latitude)
    , longitude(other.longitude)
{}

Coordinate::Coordinate()
    : d(new Private)
{}

Coordinate::Coordinate(double latitude, double longitude)
    : d(new Private(latitude, longitude))
{}

Coordinate::Coordinate(const QString &latlng)
    : d(new Private(latlng))
{}

Coordinate::Coordinate(const Coordinate &other)
    : d(other.d)
{}

Coordinate::~Coordinate()
{}

Coordinate &Coordinate::operator =(const Coordinate &other)
{
    d = other.d;
    return *this;
}

bool Coordinate::operator ==(const Coordinate &other) const
{
    if (d == other.d) return true;
    return qFuzzyCompare(d->latitude, other.d->latitude) && qFuzzyCompare(d->longitude, other.d->longitude);
}

double Coordinate::latitude() const
{
    return d->latitude;
}

void Coordinate::setLatitude(double latitude)
{
    d->latitude = latitude;
}

double Coordinate::longitude() const
{
    return d->longitude;
}

void Coordinate::setLongitude(double longitude)
{
    d->longitude = longitude;
}

QDebug operator<<(QDebug dbg, const Coordinate &coordinate)
{
    QDebugStateSaver saver(dbg);
    dbg.nospace().noquote();
    dbg << "Coordinate(";
    dbg << QString::number(coordinate.latitude(), 'f', 6);
    dbg << ", ";
    dbg << QString::number(coordinate.longitude(), 'f', 6);
    dbg << ")";
    return dbg;
}
