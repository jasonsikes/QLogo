#include "controller/logocontrollergui.h"
#include <QCoreApplication>
#include <QCommandLineParser>
#include <unistd.h>


// Some global options.
// Use 'extern' to access them.
bool hasGUI = false;
QString helpdb;
bool loadLibrary = true;

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
      {"nolib",
       QCoreApplication::translate("main",
                                   "Tell QLogo not to load the library. Library commands will "
                                   "be unavailable.")},
      {"helpdb",
       QCoreApplication::translate("main", "Specify the location of the help database."),
       QCoreApplication::translate("main", "help_database")},
                     });

  commandlineParser.process(*a);

  if (commandlineParser.isSet("QLogoGUI")) {
      hasGUI = true;
    }

  if (commandlineParser.isSet("nolib")) {
      loadLibrary = false;
  }

  if (commandlineParser.isSet("helpdb")) {
      helpdb = commandlineParser.value("helpdb");
  }
}


int main(int argc, char **argv)
{
  QCoreApplication application(argc, argv);

  processOptions(&application);

  LogoController *mainController;
  if (hasGUI) {
      mainController = new LogoControllerGUI;
  } else {
      mainController = new LogoController;
  }
  int retval = mainController->run();
  delete mainController;
  return retval;
}
