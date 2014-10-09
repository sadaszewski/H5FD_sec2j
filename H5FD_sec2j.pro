TEMPLATE=lib

CONFIG -= qt

CONFIG += debug

INCLUDEPATH += . /opt/local/include

HEADERS += H5FD_sec2j.h

SOURCES += H5FD_sec2j.c

LIBS += -L/opt/local/lib -lhdf5
