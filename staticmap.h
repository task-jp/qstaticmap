#ifndef STATICMAP_H
#define STATICMAP_H

#include <QtCore/QObject>
#include <QtCore/QUrl>
#include <QtGui/QImage>
#include "coordinate.h"

class StaticMap : public QObject
{
    Q_OBJECT
    Q_PROPERTY(Coordinate center READ center WRITE setCenter NOTIFY centerChanged)
    Q_PROPERTY(int zoom READ zoom WRITE setZoom NOTIFY zoomChanged)
    Q_PROPERTY(QSize size READ size WRITE setSize NOTIFY sizeChanged)
public:
    struct Image {
        QUrl url;
        Coordinate coordinate;
    };
    struct Text {
        QString text;
        Coordinate coordinate;
    };
    struct Path {
        QList<Coordinate> coordinates;
        QColor color;
        struct {
            QColor color;
            int width;
        } border;
    };
    explicit StaticMap(QObject *parent = nullptr);
    ~StaticMap() override;

    QImage render();

    Coordinate center() const;
    int zoom() const;
    QSize size() const;

public slots:
    void setCenter(const Coordinate &center);
    void setZoom(int zoom);
    void setSize(const QSize &size);

    void addImage(const Image &image);
    void addText(const Text &text);
    void addPath(const Path &path);

signals:
    void centerChanged(const Coordinate &center);
    void zoomChanged(int zoom);
    void sizeChanged(const QSize &size);

private:
    class Private;
    Private *d;
};

Q_DECLARE_METATYPE(StaticMap::Image)
Q_DECLARE_METATYPE(StaticMap::Text)
Q_DECLARE_METATYPE(StaticMap::Path)

#endif // STATICMAP_H