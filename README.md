# qstaticmap

google static map like service based on [QtHttpServer](https://code.qt.io/cgit/qt-labs/qthttpserver.git/)

# requirement
- Qt 5.12 or later
- QtHttpServer latest

# build and run

```
$ qmake
$ make
$ ./qstaticmap -platform offscreen // or minimal on mac
```

# try

http://127.0.0.1:9100/?size=512x512&zoom=18&center=43.039498,141.313663&images=icon:https://developers.google.com/maps/documentation/javascript/examples/full/images/beachflag.png|43.039498,141.313663&labels=text:HERE|43.039498,141.313663

