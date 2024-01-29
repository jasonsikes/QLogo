#include "qlogocontroller.h"
#include <QCoreApplication>
#include <QCommandLineParser>


// Some global options.
// Use 'extern' to access them.
bool hasGUI = false;
QString helpdb;

void processOptions(QCoreApplication *a)
{
  QCommandLineParser parser;

  QCoreApplication::setApplicationName("qlogo");
  QCoreApplication::setApplicationVersion(LOGOVERSION);

  parser.setApplicationDescription("UCBLOGO-compatable Logo language Interpreter.");
  parser.addHelpOption();
  parser.addVersionOption();

  parser.addOptions({
      {"QLogoGUI",
       QCoreApplication::translate("main",
                                   "DO NOT USE! Set the input and output to the format used by "
                                   "the QLogo GUI Application. Useless elsewhere.")},
      {"helpdb",
       QCoreApplication::translate("main", "Specify the location of the help database."),
       QCoreApplication::translate("main", "help_database")},
                     });

  parser.process(*a);

  if (parser.isSet("QLogoGUI")) {
      hasGUI = true;
    }

  if (parser.isSet("helpdb")) {
      helpdb = parser.value("helpdb");
  }
}


int main(int argc, char **argv)
{
  QCoreApplication application(argc, argv);

  processOptions(&application);

  LogoController *mainController;
  if (hasGUI) {
      mainController = new QLogoController;
  } else {
      mainController = new LogoController;
  }
  int retval = mainController->run();
  delete mainController;
  return retval;
}
