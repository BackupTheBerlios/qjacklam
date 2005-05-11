# File generated by kdevelop's qmake manager. 
# ------------------------------------------- 
# Subdir relative project main directory: ./src
# Target is an application:  ../bin/qjacklam

TARGET=../bin/qjacklam
TEMPLATE = app
LANGUAGE = C++
CONFIG += release thread qt
LIBS += -ljack
SOURCES += qlam.cpp \
           measure.cpp \
           jack.cpp \
           tools.cpp 
HEADERS += jack.h \
           tools.h \
           MainWindow.ui.h \
           menu.h \
           userevent.h
FORMS += MainWindow.ui 
