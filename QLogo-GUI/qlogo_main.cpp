//===-- qlogo/main.cpp -------*- C++ -*-===//
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

#include "gui/mainwindow.h"
#include <QApplication>
#include <QSqlDatabase>

int main(int argc, char *argv[]) {
  QApplication a(argc, argv);
  MainWindow w;
  w.show();

  return a.exec();
}


// This code isn't actually executed. It's to compel the linker to include the
// SQL stuff for the qlogo binary into the MacOS bundle.
void ignoreMe(void)
{
  QSqlDatabase *driver = new QSqlDatabase();
  delete driver;
}
