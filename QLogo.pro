#----------- qlogo/QLogo.pro ---------------------
#
# This file is part of QLogo.
#
# QLogo is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 2 of the License, or
# (at your option) any later version.
#
# QLogo is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with QLogo.  If not, see <http:#www.gnu.org/licenses/>.
#
#-------------------------------------------------
#
# Project created by QtCreator 2016-12-23T20:02:17
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = QLogo
TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any feature of Qt which as been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

DEFINES += CONTROLLER_HEADER=\\\"qlogo_controller.h\\\"

DEFINES += LOGOVERSION=\\\"0.92\\\"

win32 {
    DEFINES += LOGOPLATFORM=\\\"WINDOWS\\\"
    RC_ICONS = icon.ico
}
unix:!macx {
    DEFINES += LOGOPLATFORM=\\\"UNIX\\\"
}
macx {
    DEFINES += LOGOPLATFORM=\\\"OSX\\\"
    ICON = Icon.icns
}

SOURCES += qlogo_main.cpp\
        mainwindow.cpp \
    canvas.cpp \
    datum.cpp \
    qlogo_controller.cpp \
    console.cpp \
    parser.cpp \
    turtle.cpp \
    vars.cpp \
    kernel.cpp \
    propertylists.cpp \
    kernel_datastructureprimitives.cpp \
    kernel_communication.cpp \
    kernel_arithmetic.cpp \
    kernel_graphics.cpp \
    kernel_workspacemanagement.cpp \
    workspace.cpp \
    procedurehelper.cpp \
    editorwindow.cpp \
    help.cpp \
    kernel_controlstructures.cpp \
    error.cpp \
    library.cpp \
    datum_word.cpp \
    datum_astnode.cpp \
    datum_list.cpp \
    datum_array.cpp \
    datum_datump.cpp \
    datum_iterator.cpp


HEADERS  += mainwindow.h \
    canvas.h \
    datum.h \
    qlogo_controller.h \
    console.h \
    parser.h \
    turtle.h \
    vars.h \
    kernel.h \
    propertylists.h \
    workspace.h \
    procedurehelper.h \
    editorwindow.h \
    help.h \
    error.h \
    library.h

FORMS    += mainwindow.ui \
    editorwindow.ui

# Windows seems to have an issue with OpenGL

win32 {
    LIBS += -lOpengl32
}

# CONFIG += console
CONFIG += c++11

