#ifndef COORDINATE_H
#define COORDINATE_H

#include <QtCore/QSharedDataPointer>

class Coordinate
{
public:
    Coordinate();
    Coordinate(double latitude, double longitude);
    Coordinate(const QString &latlng);
    Coordinate(const Coordinate &other);
    ~Coordinate();

    Coordinate &operator=(const Coordinate &other);
    void swap(Coordinate &other) Q_DECL_NOTHROW { qSwap(d, other.d); }

    bool operator==(const Coordinate &other) const;

    double latitude() const;
    void setLatitude(double latitude);
    double longitude() const;
    void setLongitude(double longitude);

    double x() const { return longitude(); }
    double y() const { return latitude(); }

private:
    class Private;
    QSharedDataPointer<Private> d;
};

Q_DECLARE_SHARED(Coordinate)

QDebug operator<<(QDebug, const Coordinate &);

#endif // COORDINATE_H