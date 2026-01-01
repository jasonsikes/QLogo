// Copyright 2017-2024 Jason Sikes
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted under the conditions specified in the
// license found in the LICENSE file in the project root.

#include "gui/mainwindow.h"
#include <QApplication>
#include <QSqlDatabase>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    w.show();

    return a.exec();
}

// This code isn't actually executed. It's to compel the linker to include the
// SQL stuff for the qlogo binary into the MacOS bundle.
// cppcheck-suppress unusedFunction
void ignoreMe(void)
{
    QSqlDatabase *driver = new QSqlDatabase();
    delete driver;
}
