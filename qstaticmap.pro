requires(qtHaveModule(httpserver))

TEMPLATE = app
QT += httpserver
CONFIG += console
CONFIG -= app_bundle

HEADERS += \
    coordinate.h \
    staticmap.h \
    urlqueryparser.h

SOURCES += \
    main.cpp \
    coordinate.cpp \
    staticmap.cpp \
    urlqueryparser.cpp

RESOURCES += \
    fonts.qrc