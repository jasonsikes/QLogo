//===-- qlogo/logo_main.cpp - main function for QLogo -------*- C++ -*-===//
//
// Copyright 2017-2024 Jason Sikes
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted under the conditions specified in the
// license found in the LICENSE file in the project root.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the main function for the QLogo language interpreter. Upon
/// initialization, the main function will process command line arguments and create
/// a LogoController (or LogoController subclass) object. The main function will then
/// call the run() method of the LogoController object, which will start the main
/// loop of the QLogo language interpreter.
///
//===----------------------------------------------------------------------===//

#include "controller/logocontrollergui.h"
#include <QCommandLineParser>
#include <QCoreApplication>
#include <unistd.h>

/// Process command line options and set the configuration accordingly.
///
/// @param a The QCoreApplication object.
void processOptions(QCoreApplication *a)
{
    QCommandLineParser commandlineParser;

    QCoreApplication::setApplicationName("qlogo");
    QCoreApplication::setApplicationVersion(LOGOVERSION);

    commandlineParser.setApplicationDescription("UCBLOGO-compatable Logo language Interpreter.");
    commandlineParser.addHelpOption();
    commandlineParser.addVersionOption();

    commandlineParser.addOptions({
        {"QLogoGUI",
         QCoreApplication::translate("main",
                                     "DO NOT USE! Sets the input and output to the format used by "
                                     "the QLogo GUI Application. Will result in garbage being printed "
                                     "out to your terminal.")},
        {"setlibloc", QCoreApplication::translate("main", "Specify the location of the standard library database.")},
        {"sethelploc",
         QCoreApplication::translate("main", "Specify the location of the help database."),
         QCoreApplication::translate("main", "help_database")},
    });

    commandlineParser.process(*a);

    if (commandlineParser.isSet("QLogoGUI"))
    {
        Config::get().hasGUI = true;
    }

    if (commandlineParser.isSet("setlibloc"))
    {
        Config::get().paramLibraryDatabaseFilepath = commandlineParser.value("setlibloc");
    }

    if (commandlineParser.isSet("sethelploc"))
    {
        Config::get().paramHelpDatabaseFilepath = commandlineParser.value("sethelploc");
    }
}

int main(int argc, char **argv)
{
    // Pass all the command line arguments to Config in case they are queried later.
    for (int i = 0; i < argc; ++i)
    {
        Config::get().ARGV.push_back(QString(argv[i]));
    }

    QCoreApplication application(argc, argv);

    processOptions(&application);

    LogoController *mainController;
    if (Config::get().hasGUI)
    {
        mainController = new LogoControllerGUI;
    }
    else
    {
        mainController = new LogoController;
    }
    int retval = mainController->run();
    delete mainController;
    return retval;
}
