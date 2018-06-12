#include "message.h"

//
// C_CONSOLE_PRINT_STRING
//

const QString consolePrintStringFromMessage(const QByteArray message)
{
    const char *data = message.constData();
    int *length = (int*)&data[1];
    const QChar *str = (const QChar *)&data[1 + sizeof(int)];
    return QString::fromRawData(str, *length);
}

const QByteArray messageFromConsolePrintString(const QString str)
{
  int length = str.length();
  QByteArray message(1, C_CONSOLE_PRINT_STRING);
  message.append((char*)&length, sizeof(int));
  message.append((char*)str.constData(), length * sizeof(QChar));
  return message;
}

//
// C_CONSOLE_SET_TEXT_SIZE
//

double consoleSetTextSizeFromMessage(const QByteArray message)
{
  const char *data = message.constData();

  double *val = (double*)&data[1];
  return *val;
}

const QByteArray messageFromConsoleSetTextSize(double size){
  QByteArray message(1, C_CONSOLE_SET_TEXT_SIZE);
  message.append((char*)&size, sizeof(double));
  return message;
}

//
// C_CONSOLE_SET_CURSOR_POS
//

QVector<int> consoleSetCursorPosFromMessage(const QByteArray message)
{
  const char *data = message.constData();

  int *row = (int*)&data[1];
  int *column = (int*)&data[1 + sizeof(int)];

  QVector<int> retval;
  retval << *row;
  retval << *column;
  return retval;
}

const QByteArray messageFromConsoleSetCursorPos(QVector<int> position)
{
  QByteArray message(1, C_CONSOLE_SET_CURSOR_POS);
  message.append((char*)&position.first(), sizeof(int));
  message.append((char*)&position.last(), sizeof(int));
  return message;
}

//
// C_CONSOLE_SET_TEXT_COLOR
//

QVector<QColor> consoleSetTextColorFromMessage(const QByteArray message)
{
  const char *data = message.constData();

  QRgba64 *foreground = (QRgba64 *) &data[1];
  QRgba64 *background = (QRgba64 *) &data[1 + sizeof(QRgba64)];


  QVector<QColor> retval;
  retval << QColor(*foreground);
  retval << QColor(*background);
  return retval;
}

const QByteArray messageFromConsoleSetTextColor(QVector<QColor> colors)
{
  QRgba64 foreground = colors.first().rgba64();
  QRgba64 background = colors.last().rgba64();

  QByteArray message(1, C_CONSOLE_SET_TEXT_COLOR);
  message.append((char*)&foreground, sizeof(QRgba64));
  message.append((char*)&background, sizeof(QRgba64));
  return message;
}

//
// C_CONSOLE_CLEAR_TEXT
//

const QByteArray messageFromConsoleClearText(void)
{
  QByteArray message(1, C_CONSOLE_CLEAR_TEXT);
  return message;
}
