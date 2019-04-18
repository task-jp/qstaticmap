#include "staticmap.h"

#include <QtCore/QVariant>
#include <QtCore/QFileInfo>
#include <QtCore/QDir>
#include <QtCore/QStandardPaths>

#include <QtGui/QPainter>
#include <QtGui/QFontMetrics>

#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QNetworkReply>

#include <cmath>

// https://github.com/systemed/tilemaker/blob/master/src/coordinates.cpp
double deg2rad(double deg) { return (M_PI/180.0) * deg; }
double rad2deg(double rad) { return (180.0/M_PI) * rad; }

// Project latitude (spherical Mercator)
// (if calling with raw coords, remember to divide/multiply by 10000000.0)
double lat2latp(double lat) { return rad2deg(log(tan(deg2rad(lat+90.0)/2.0))); }
double latp2lat(double latp) { return rad2deg(atan(exp(deg2rad(latp)))*2.0)-90.0; }

// Tile conversions
double lon2tilexf(double lon, int z) { return scalbn((lon+180.0) * (1/360.0), z); }
double latp2tileyf(double latp, int z) { return scalbn((180.0-latp) * (1/360.0), z); }
double lat2tileyf(double lat, int z) { return latp2tileyf(lat2latp(lat), z); }
int lon2tilex(double lon, int z) { return static_cast<int>(lon2tilexf(lon, z)); }
int latp2tiley(double latp, int z) { return static_cast<int>(latp2tileyf(latp, z)); }
int lat2tiley(double lat, int z) { return static_cast<int>(lat2tileyf(lat, z)); }
double tilex2lon(int x, int z) { return scalbn(x, -z) * 360.0 - 180.0; }
double tiley2latp(int y, int z) { return 180.0 - scalbn(y, -z) * 360.0; }
double tiley2lat(int y, int z) { return latp2lat(tiley2latp(y, z)); }

class StaticMap::Private
{
public:
    Private();
    QImage tile(int x, int y, int z);
    QImage fetch(const QUrl &url);
    Coordinate center;
    int zoom;
    QSize size;

    QList<QVariant> items;
    static QHash<QString, QImage> imageCache;
    static QNetworkAccessManager nam;
    QDir cacheDir;
    QString tileUrl;
    QString copyright;
};

QHash<QString, QImage> StaticMap::Private::imageCache;
QNetworkAccessManager StaticMap::Private::nam;

StaticMap::Private::Private()
    : zoom(0)
    , cacheDir(QStandardPaths::writableLocation(QStandardPaths::CacheLocation))
    , tileUrl(qEnvironmentVariable("TILE_URL", QStringLiteral("https://a.tile.openstreetmap.org/{z}/{x}/{y}.png")))
    , copyright(qEnvironmentVariable("TILE_COPYRIGHT", QStringLiteral("© OpenStreetMap contributors")))
{
    if (!cacheDir.exists()) {
        cacheDir.mkpath(".");
    }
}

QImage StaticMap::Private::tile(int x, int y, int z)
{
    QString url = tileUrl;
    url.replace(QStringLiteral("{x}"), QString::number(x))
            .replace(QStringLiteral("{y}"), QString::number(y))
            .replace(QStringLiteral("{z}"), QString::number(z));
    return fetch(QUrl(url));
}

QImage StaticMap::Private::fetch(const QUrl &url)
{
    QImage ret;
    if (url.isLocalFile()) return ret;

    QString key = url.toString().toUtf8().toHex();
    if (imageCache.contains(key)) {
        ret = imageCache.value(key);
    } else {
        QString cache = QStandardPaths::locate(QStandardPaths::CacheLocation, key);
        if (!cache.isEmpty()) {
            ret = QImage(cache);
        } else {
            QString format = QFileInfo(url.path()).suffix();
            if (url.scheme() == QStringLiteral("qrc")) {
                QFile file(QStringLiteral(":") + url.toString().mid(6));
                if (file.open(QFile::ReadOnly)) {
                    ret = QImage::fromData(file.readAll(), qPrintable(format));
                    file.close();
                }
            } else {
                QNetworkRequest request(url);
                request.setHeader(QNetworkRequest::UserAgentHeader, QByteArrayLiteral("QGeoTileFetcher"));
                request.setAttribute(QNetworkRequest::SynchronousRequestAttribute, true);
                QNetworkReply *reply = nam.get(request);
                ret = QImage::fromData(reply->readAll(), qPrintable(format));
            }
            ret.save(cacheDir.filePath(key), qPrintable(format));
        }
        imageCache.insert(key, ret);
    }
    return ret;
}

StaticMap::StaticMap(QObject *parent)
    : QObject(parent)
    , d(new Private)
{
}

StaticMap::~StaticMap()
{
    delete d;
}

QImage StaticMap::render()
{
    QImage ret(d->size.width(), d->size.height(), QImage::Format_ARGB32_Premultiplied);
    ret.fill(Qt::red);

    // determine area
    int z = zoom();
    Coordinate topLeft;
    Coordinate bottomRight;
    bool first = true;
    for (const QVariant &item : d->items) {
        if (item.canConvert<Image>()) {
            Image data = item.value<Image>();
            if (first) {
                topLeft = data.coordinate;
                bottomRight = data.coordinate;
                first = false;
            } else {
                topLeft.setLatitude(std::max(topLeft.latitude(), data.coordinate.latitude()));
                topLeft.setLongitude(std::min(topLeft.longitude(), data.coordinate.longitude()));
                bottomRight.setLatitude(std::min(bottomRight.latitude(), data.coordinate.latitude()));
                bottomRight.setLongitude(std::max(bottomRight.longitude(), data.coordinate.longitude()));
            }
        } else if (item.canConvert<Text>()) {
            Text data = item.value<Text>();
            if (first) {
                topLeft = data.coordinate;
                bottomRight = data.coordinate;
                first = false;
            } else {
                topLeft.setLatitude(std::max(topLeft.latitude(), data.coordinate.latitude()));
                topLeft.setLongitude(std::min(topLeft.longitude(), data.coordinate.longitude()));
                bottomRight.setLatitude(std::min(bottomRight.latitude(), data.coordinate.latitude()));
                bottomRight.setLongitude(std::max(bottomRight.longitude(), data.coordinate.longitude()));
            }
        } else if (item.canConvert<Path>()) {
            Path data = item.value<Path>();
            QPolygonF polygon;
            for (const Coordinate &coordinate : data.coordinates) {
                if (first) {
                    topLeft = coordinate;
                    bottomRight = coordinate;
                    first = false;
                } else {
                    topLeft.setLatitude(std::max(topLeft.latitude(), coordinate.latitude()));
                    topLeft.setLongitude(std::min(topLeft.longitude(), coordinate.longitude()));
                    bottomRight.setLatitude(std::min(bottomRight.latitude(), coordinate.latitude()));
                    bottomRight.setLongitude(std::max(bottomRight.longitude(), coordinate.longitude()));
                }
            }
        } else {
            qWarning() << item;
        }
    }

    {
        int tilesx = lon2tilex(topLeft.longitude(), z) - lon2tilex(bottomRight.longitude(), z);
        int tilesy = lat2tiley(topLeft.latitude(), z) - lat2tiley(bottomRight.latitude(), z);
        int tiles = std::max(std::abs(tilesx), std::abs(tilesy));
        Coordinate c = center();
        if (qFuzzyIsNull(c.latitude()) && qFuzzyIsNull(c.longitude())) {
            Coordinate c((topLeft.latitude() + bottomRight.latitude()) / 2,
                    (topLeft.longitude() + bottomRight.longitude()) / 2);
            int cx = lon2tilex(c.longitude(), z);
            int cy = lat2tiley(c.latitude(), z);
            double w = std::abs(tilex2lon(cx + 1, z) - tilex2lon(cx, z)) * d->size.width() / 256;
            double h = std::abs(tiley2lat(cy + 1, z) - tiley2lat(cy, z)) * d->size.height() / 256;
            topLeft = Coordinate(c.latitude() + tiles * 0.5 * h, c.longitude() - tiles * 0.5 * w);
            bottomRight = Coordinate(c.latitude() - tiles * 0.5 * h, c.longitude() + tiles * 0.5 * w);
        } else {
            // todo
            int cx = lon2tilex(c.longitude(), z);
            int cy = lat2tiley(c.latitude(), z);
            double w = std::abs(tilex2lon(cx + 1, z) - tilex2lon(cx, z)) * d->size.width() / 256;
            double h = std::abs(tiley2lat(cy + 1, z) - tiley2lat(cy, z)) * d->size.height() / 256;
            tiles = 1;
            topLeft = Coordinate(c.latitude() + tiles * 0.5 * h, c.longitude() - tiles * 0.5 * w);
            bottomRight = Coordinate(c.latitude() - tiles * 0.5 * h, c.longitude() + tiles * 0.5 * w);
        }
    }

    // fill tiles
    int x1 = lon2tilex(topLeft.longitude(), z);
    int x2 = lon2tilex(bottomRight.longitude(), z);
    int y1 = lat2tiley(topLeft.latitude(), z);
    int y2 = lat2tiley(bottomRight.latitude(), z);
    int xmin = std::min(x1, x2);
    int xmax = std::max(x1, x2) + 1;
    int ymin = std::min(y1, y2);
    int ymax = std::max(y1, y2) + 1;

    QPainter painter;
    painter.begin(&ret);
    int margin = 0;
#define LONGITUDE2X(l) \
    ((l - topLeft.longitude()) / std::abs(topLeft.longitude() - bottomRight.longitude()) * d->size.width())
#define LATITUDE2Y(l) \
    ((topLeft.latitude() - l) / std::abs(topLeft.latitude() - bottomRight.latitude()) * d->size.height())

    for (int y = ymin + margin; y < ymax - margin; y++) {
        for (int x = xmin + margin; x < xmax - margin; x++) {
            QImage tile = d->tile(x, y, z);
            QPointF nw(LONGITUDE2X(tilex2lon(x, z)), LATITUDE2Y(tiley2lat(y, z)));
            QPointF se(LONGITUDE2X(tilex2lon(x + 1, z)), LATITUDE2Y(tiley2lat(y + 1, z)));
            QRect rect(std::round(nw.x()), std::round(nw.y()), std::round(se.x()) - std::round(nw.x()), std::round(se.y()) - std::round(nw.y()));
            painter.drawImage(rect, tile);
//            painter.drawRoundedRect(rect, 10, 10);
        }
    }

    // fill others
    painter.setRenderHint(QPainter::Antialiasing);
    QFontMetrics f(painter.font());
    for (const QVariant &item : d->items) {
        if (item.canConvert<Image>()) {
            Image data = item.value<Image>();
            QImage image = d->fetch(data.url);
            int w = image.width();
            int h = image.height();
            QPointF pos(LONGITUDE2X(data.coordinate.longitude()), LATITUDE2Y(data.coordinate.latitude()));
            painter.drawImage(pos.x() - w / 2, pos.y() - h / 2 , image);
        } else if (item.canConvert<Text>()) {
            Text data = item.value<Text>();
            int w = f.width(data.text);
            int h = f.height() * (data.text.count(QLatin1Char('\n')) + 1);
            QPointF pos(LONGITUDE2X(data.coordinate.longitude()), LATITUDE2Y(data.coordinate.latitude()));
            painter.drawText(pos.x() - w / 2, pos.y() - h / 2 , w, h, Qt::AlignCenter, data.text);
        } else if (item.canConvert<Path>()) {
            painter.save();
            Path data = item.value<Path>();
            QPen pen;
            pen.setColor(data.border.color);
            pen.setWidth(data.border.width);
            painter.setPen(pen);
            painter.setBrush(data.color);
            QPolygonF polygon;
            for (const Coordinate &coordinate : data.coordinates) {
                polygon.append(QPointF(LONGITUDE2X(coordinate.longitude()), LATITUDE2Y(coordinate.latitude())));
            }
            painter.drawPolygon(polygon);
            painter.restore();
        } else {
            qWarning() << item;
        }
    }
#undef LONGITUDE2X
#undef LATITUDE2Y

    // copyrights
    {
        int w = f.width(d->copyright) + 4;
        int h = f.height() + 2;
        painter.fillRect(d->size.width() - w, d->size.height() - h, w, h, QBrush(Qt::lightGray));
        painter.drawText(d->size.width() - w, d->size.height() - h, w, h, Qt::AlignCenter, d->copyright);
    }
    painter.end();
    return ret;
}

void StaticMap::addImage(const Image &image)
{
    d->items.append(QVariant::fromValue(image));
}

void StaticMap::addText(const Text &text)
{
    d->items.append(QVariant::fromValue(text));
}

void StaticMap::addPath(const Path &path)
{
    d->items.append(QVariant::fromValue(path));
}

Coordinate StaticMap::center() const
{
    return d->center;
}

void StaticMap::setCenter(const Coordinate &center)
{
    if (d->center == center) return;
    d->center = center;
    emit centerChanged(center);
}

int StaticMap::zoom() const
{
    return d->zoom;
}

void StaticMap::setZoom(int zoom)
{
    if (d->zoom == zoom) return;
    d->zoom = zoom;
    emit zoomChanged(zoom);
}

QSize StaticMap::size() const
{
    return d->size;
}

void StaticMap::setSize(const QSize &size)
{
    if (d->size == size) return;
    d->size = size;
    emit sizeChanged(size);
}

