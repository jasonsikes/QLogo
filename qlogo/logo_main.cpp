#include "controller/logocontrollergui.h"
#include <QCoreApplication>
#include <QCommandLineParser>
#include <unistd.h>


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
                                   "DO NOT USE! Set the input and output to the format used by "
                                   "the QLogo GUI Application. Useless elsewhere.")},
      {"setlibloc",
       QCoreApplication::translate("main",
                                   "Specify the location of the standard library database.")},
      {"sethelploc",
       QCoreApplication::translate("main", "Specify the location of the help database."),
       QCoreApplication::translate("main", "help_database")},
                     });

  commandlineParser.process(*a);

  if (commandlineParser.isSet("QLogoGUI")) {
      Config::get().hasGUI = true;
    }

  if (commandlineParser.isSet("setlibloc")) {
        Config::get().paramLibraryDatabaseFilepath = commandlineParser.value("setlibloc");
  }

  if (commandlineParser.isSet("sethelploc")) {
      Config::get().paramHelpDatabaseFilepath = commandlineParser.value("sethelploc");
  }
}


int main(int argc, char **argv)
{
    // Pass all the command line arguments to Config in case they are queried later.
    for (int i = 0; i < argc; ++i) {
        Config::get().ARGV.push_back(QString(argv[i]));
    }

    QCoreApplication application(argc, argv);

    processOptions(&application);

    LogoController *mainController;
    if (Config::get().hasGUI) {
        mainController = new LogoControllerGUI;
    } else {
        mainController = new LogoController;
    }
    int retval = mainController->run();
    delete mainController;
    return retval;
}
