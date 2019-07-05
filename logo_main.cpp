#include "qlogocontroller.h"
#include "logocontroller.h"
#include <QCoreApplication>
#include <QCommandLineParser>


// Some global options.
// Use 'extern' to access them.
bool hasGUI = false;

void processOptions(QCoreApplication *a)
{
  QCommandLineParser parser;

  QCoreApplication::setApplicationName("logo");
  QCoreApplication::setApplicationVersion(LOGOVERSION);

  parser.setApplicationDescription("UCBLOGO-compatable Logo language Interpreter.");
  parser.addHelpOption();
  parser.addVersionOption();

  parser.addOptions({
                      {"QLogoGUI",
                       QCoreApplication::translate("main",
                       "Set the input and output to the format used by the QLogo GUI Application. "
                       "This option is meant to be set by the QLogo Application which "
                       "communicates with logo using QLogo as a front end.")},
                    });

  parser.process(*a);

  if (parser.isSet("QLogoGUI")) {
      hasGUI = true;
    }
}


int main(int argc, char **argv)
{
  QCoreApplication application(argc, argv);

  processOptions(&application);

  Controller *mainController;
  if (hasGUI) {
      mainController = new QLogoController;
  } else {
      mainController = new LogoController;
  }
  int retval = mainController->run();
  delete mainController;
  return retval;
}
