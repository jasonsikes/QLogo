#include <QSqlError>
#include <QSqlQuery>
#include <QStringList>
#include <QFile>
#include <QDir>
#include <QTextStream>
#include <iostream>

//===-- qlogo/gen_help.cpp - Help generator -------*- C++ -*-===//
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
/// This file generates a SQLite database of help text, which is used
/// by the HELP command in QLogo.
///
//===----------------------------------------------------------------------===//

using namespace std;

void showUsage(const char *argv0)
{
    cout << "USAGE: " <<argv0 << " <db_filename> <srcdir>\n";
    cout << "WHERE db_filename is the path of the SQLite database you wish to create.\n";
    cout << "      srcdir is the path of the source directory containing the\n";
    cout << "   helptext entries.\n";
}

// The database hande used throughout the program.
QSqlDatabase db;

// The SQL text that will be used to enter data into the ALIASES table.
QSqlQuery aliasesQuery;

// The SQL text that will be used to enter data into the HELPTEXT table.
QSqlQuery helptextQuery;

/*
 * Find the next header in the input stream.
 * A documentation header line starts with a slash and the string "***DOC".
 * It is then followed by one or more command names, e.g.:
 * "[slash]***DOC FORWARD FD".
 * The command names are returned. In the case of EOF an empty stringlist
 * is returned.
 */
QStringList findNextDocHeader(QTextStream &stream)
{
    QStringList retval;

    while ( ! stream.atEnd()) {
        QString line = stream.readLine().trimmed();
        if (line.startsWith(u"/***DOC")) {
            line.remove(0,8); // remove '/'[0],"***"[1,2,3], "DOC"[4,5,6], space[7]
            retval = line.split(' ');
            break;
        }
    }

    return retval;
}


/*
 * Read in the text of a documentation entry.
 * Reads and collects text until end of documentation marker:
 * "COD***[slash]".
 * Returns the entire help text as a string.
 */
QString readText(QTextStream &stream)
{
    QString retval;

    forever {
        QString line = stream.readLine();
        if (line.isNull() || line.contains(u"COD***/")) {
            break;
        }
        retval += line + "\n";
    }

    return retval;
}


/*
 * Insert an entry into the documentation database
 */
void insertDB(QStringList aliases, QString helptext)
{
    // The first element of the aliases will be the key.
    QString cmd = aliases[0];

    for (auto &alias : aliases) {
        aliasesQuery.bindValue(0, alias);
        aliasesQuery.bindValue(1, cmd);
        if ( ! aliasesQuery.exec()) {
            cout << "Problem with ALIASES query\n";
            exit(0);
        }
    }

    helptextQuery.bindValue(0, cmd);
    helptextQuery.bindValue(1, helptext);
    if ( ! helptextQuery.exec()) {
        cout << "Problem with HELPTEXT query\n";
        exit(0);
    }
}

/*
 * Initialize the database and prepare the database queries that
 * will be used throughout the program.
 * Returns true if successful, false otherwise.
 */
bool initDB(QString filename)
{
    QFileInfo fileInfo(filename);
    QFile::remove(filename);
    fileInfo.absolutePath();
    db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName(filename);

    if ( ! db.open()) {
        cout <<"DB Error: " << db.lastError().text().toStdString() << " \n";
        return false;
    }

    QSqlQuery q(db);
    if ( ! q.exec("CREATE TABLE ALIASES"
                "(ALIAS TEXT PRIMARY KEY NOT NULL,"
                "COMMAND TEXT NOT NULL)")

        || ! q.exec("CREATE TABLE HELPTEXT"
                   "(COMMAND TEXT PRIMARY KEY NOT NULL,"
                   "DESCRIPTION TEXT)")) {
        cout <<"Problem creating tables: " << db.lastError().text().toStdString() << " \n";
        return false;
    }

    aliasesQuery = QSqlQuery(db);
    helptextQuery = QSqlQuery(db);

    if ( ! aliasesQuery.prepare("INSERT INTO ALIASES"
                              " (ALIAS,COMMAND)"
                              " VALUES (?,?)")
        || ! helptextQuery.prepare("INSERT INTO HELPTEXT"
                                 " (COMMAND,DESCRIPTION)"
                                 " VALUES (?,?)")) {
        cout <<"Problem creating insert queries: " << db.lastError().text().toStdString() << " \n";
        return false;
    }

    return true;
}


/*
 * Return a list of all the CPP source files in given directory.
 */
QStringList filesInDir(QString dirpath)
{
    QDir dir(dirpath);
    if ( ! dir.exists())
        return QStringList();

    QStringList filter;
    filter <<"*.cpp";
    return dir.entryList(filter);
}


int main(int argc, char *argv[])
{
    if (argc != 3) {
        showUsage(argv[0]);
        return 1;
    }

    QString filename(argv[1]);
    QString srcDir  (argv[2]);

    QStringList fileList = filesInDir(srcDir);
    if (fileList.size() < 1) {
        cout << "Bad source directory path.\n";
        showUsage(argv[0]);
        return 1;
    }

    if ( ! initDB(filename)) {
        return 1;
    }
    int totalEntries = 0;

    for (auto &srcFileName : fileList) {
        QString filePath = srcDir + QDir::separator() + srcFileName;
        QFile file(filePath);
        cout <<"Reading: " << filePath.toStdString() << "\n";
        int entries = 0;
        if ( ! file.open(QIODevice::ReadOnly)) {
            cout << "Could not open!\n";
            return 1;
        }
        QTextStream stream(&file);
        forever {
            QStringList aliases = findNextDocHeader(stream);
            if (aliases.size() < 1)
                break;
            QString text = readText(stream);
            ++entries; ++totalEntries;
            insertDB(aliases, text);
        }
        cout << "Entries: " << entries << "\n";
    }

    db.close();
    db.removeDatabase("QSQLITE");
    cout << "Total entries: " <<totalEntries << "\n";

    return 0;
}
