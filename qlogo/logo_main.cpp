//===-- qlogo/logo_main.cpp - main function for QLogo -------*- C++ -*-===//
//
// This file is part of QLogo.
//
// QLogo is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// QLogo is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with QLogo.  If not, see <http://www.gnu.org/licenses/>.
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
