// Copyright 2017-2024 Jason Sikes
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted under the conditions specified in the
// license found in the LICENSE file in the project root.

#include "gui/mainwindow.h"
#include <QApplication>
#include <QCommandLineParser>
#include <QSqlDatabase>

// Global logging flag
bool logging = false;

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // Parse command line arguments
    QCommandLineParser parser;
    parser.addOption(QCommandLineOption("log", "Enable logging"));
    parser.process(a);

    // Set logging flag if --log option is present
    if (parser.isSet("log"))
    {
        logging = true;
    }

    MainWindow w;
    w.show();

    return a.exec();
}

// This code isn't actually executed. It's to compel the linker to include the
// SQL stuff for the qlogo binary into the MacOS bundle.
// cppcheck-suppress unusedFunction
void ignoreMe()
{
    auto driver = new QSqlDatabase();
    delete driver;
}
