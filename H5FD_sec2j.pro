TEMPLATE=lib

CONFIG -= qt

INCLUDEPATH += . /opt/local/include

HEADERS += H5FD_sec2j.h

SOURCES += H5FD_sec2j.c

LIBS += -L/opt/local/lib -lhdf5
