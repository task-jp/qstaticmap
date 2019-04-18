#include "staticmap.h"
#include "urlqueryparser.h"

#include <QtCore/QDebug>
#include <QtCore/QBuffer>
#include <QtCore/QUrlQuery>

#include <QtGui/QGuiApplication>
#include <QtGui/QImage>
#include <QtGui/QFontDatabase>

#include <QtHttpServer/QHttpServer>
#include <QtHttpServer/QHttpServerRequest>

QByteArray toPng(const QImage &image) {
    QByteArray png;
    QBuffer buffer(&png);
    buffer.open(QIODevice::WriteOnly);
    image.save(&buffer, "PNG");
    buffer.close();
    return png;
}

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    QFontDatabase::addApplicationFont(":/fonts/OpenSans-Regular.ttf");

    QHttpServer server;
    server.route("/", [] (const QHttpServerRequest &request) {
        StaticMap map;
        map.setZoom(16);
        qDebug() << request.url();
        for (const auto &item : request.query().queryItems(QUrl::FullyDecoded)) {
            QString key = item.first;
            QString value = item.second;
            if (key == QStringLiteral("size")) {
                QStringList size = value.split("x");
                if (size.length() != 2) break;
                bool ok;
                int w = size[0].toInt(&ok);
                if (!ok) break;
                int h = size[1].toInt(&ok);
                if (!ok) break;
                map.setSize(QSize(w, h));
            } else if (key == QStringLiteral("center")) {
                QStringList latlng = value.split(",");
                if (latlng.length() != 2) break;
                bool ok;
                double lat = latlng[0].toDouble(&ok);
                if (!ok) break;
                double lng = latlng[1].toDouble(&ok);
                if (!ok) break;
                map.setCenter(Coordinate(lat, lng));
            } else if (key == QStringLiteral("zoom")) {
                bool ok;
                int z = value.toInt(&ok);
                if (!ok) break;
                map.setZoom(z);
            } else if (key == QStringLiteral("path")) {
                StaticMap::Path path;
                UrlQueryParser::parse(value, [&path] (const QString &key, const QString &value) {
                    if (key == QStringLiteral("color")) {
                        uint rgba = value.toUInt(nullptr, 16);
                        path.border.color = QColor::fromRgba((rgba >> 8) | (rgba & 0xff) << 24);
                    } else if (key == QStringLiteral("weight")) {
                        path.border.width = value.toInt();
                    } else if (key == QStringLiteral("fillcolor")) {
                        uint rgba = value.toUInt(nullptr, 16);
                        path.color = QColor::fromRgba((rgba >> 8) | (rgba & 0xff) << 24);
                    } else {
                        qDebug() << key << value << "not suppored";
                    }
                }, [&path](const Coordinate &coordinate) {
                    path.coordinates.append(coordinate);
                });
                map.addPath(path);
            } else if (key == QStringLiteral("images")) {
                StaticMap::Image image;
                UrlQueryParser::parse(value, [&image] (const QString &key, const QString &value) {
                    if (key == QStringLiteral("icon")) {
                        image.url = QUrl(value);
                    } else {
                        qDebug() << key << value << "not suppored";
                    }
                }, [&image, &map](const Coordinate &coordinate) {
                    image.coordinate = coordinate;
                    map.addImage(image);
                });
            } else if (key == QStringLiteral("labels")) {
                StaticMap::Text text;
                UrlQueryParser::parse(value, [&text] (const QString &key, const QString &value) {
                    if (key == QStringLiteral("text")) {
                        text.text = value;
                    } else {
                        qDebug() << key << value << "not suppored";
                    }
                }, [&text, &map](const Coordinate &coordinate) {
                    text.coordinate = coordinate;
                    map.addText(text);
                });
            } else {
                qWarning() << key << "not supported";
            }
        }
        return QHttpServerResponse(QByteArrayLiteral("image/png"), toPng(map.render()));
    });

    const auto port = server.listen(QHostAddress::LocalHost, 9100);
    if (port == -1) {
        qDebug() << QCoreApplication::translate(
                "MapRenderingServer", "Could not run on http://127.0.0.1:%1/").arg(port);
        return 0;
    }

    qDebug() << QCoreApplication::translate(
            "MapRenderingServer", "Running on http://127.0.0.1:%1/ (Press CTRL+C to quit)").arg(port);

    return app.exec();
}
