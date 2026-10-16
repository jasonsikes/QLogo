//===-- util/mklogolib.cpp - Build the Logo library SQLite database -*- C++ -*-===//
//
// Copyright 2017-2026 Jason Sikes
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted under the conditions specified in the
// license found in the LICENSE file in the project root.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// Host tool that packs Logo library procedures into qlogo_library.db.
///
/// Usage: mklogolib <compiler-dir> <output-db>
///
/// Library procedures are taken from /***LIB ... BIL***/ blocks in
/// compiler*.cpp. The name after /***LIB is the LIBRARY.COMMAND key
/// (uppercased to match Word::genKeyString()). The following lines until
/// BIL***/ are LIBRARY.CODE.
///
//===----------------------------------------------------------------------===//

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QString>
#include <QTextStream>
#include <cstdio>

namespace
{

QTextStream &err()
{
    static QTextStream stream(stderr);
    return stream;
}

bool processCompilerFile(const QFileInfo &info, QSqlQuery &query, int &entryCount)
{
    QFile file(info.absoluteFilePath());
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        err() << "Cannot read '" << info.absoluteFilePath() << "': " << file.errorString() << '\n';
        return false;
    }

    QTextStream in(&file);

    while (!in.atEnd())
    {
        const QString line = in.readLine().trimmed();
        if (!line.startsWith(QLatin1String("/***LIB")))
            continue;

        const QString command = line.mid(QStringLiteral("/***LIB").size()).trimmed().toUpper();
        if (command.isEmpty())
        {
            err() << "Malformed /***LIB header in '" << info.fileName() << "'\n";
            return false;
        }

        QString code;
        bool foundEnd = false;
        while (!in.atEnd())
        {
            const QString textLine = in.readLine();
            if (textLine.trimmed().startsWith(QLatin1String("BIL***/")))
            {
                foundEnd = true;
                break;
            }
            code += textLine;
            code += QLatin1Char('\n');
        }
        if (!foundEnd)
        {
            err() << "Unterminated /***LIB block in '" << info.fileName() << "'\n";
            return false;
        }

        query.bindValue(0, command);
        query.bindValue(1, code);
        if (!query.exec())
        {
            err() << "INSERT failed for '" << command << "': " << query.lastError().text() << '\n';
            return false;
        }
        ++entryCount;
    }

    return true;
}

bool processCompilerDir(const QString &sourceDirPath, QSqlQuery &query)
{
    QDir sourceDir(sourceDirPath);
    if (!sourceDir.exists())
    {
        err() << "Compiler directory does not exist: " << sourceDirPath << '\n';
        return false;
    }

    const QFileInfoList files =
        sourceDir.entryInfoList(QStringList{QStringLiteral("compiler*.cpp")},
                                QDir::Files | QDir::Readable | QDir::NoDotAndDotDot, QDir::Name);
    if (files.isEmpty())
    {
        err() << "No compiler*.cpp files found in " << sourceDirPath << '\n';
        return false;
    }

    int entryCount = 0;
    for (const QFileInfo &info : files)
    {
        if (!processCompilerFile(info, query, entryCount))
            return false;
    }
    if (entryCount == 0)
    {
        err() << "No /***LIB blocks found in " << sourceDirPath << '\n';
        return false;
    }
    return true;
}

} // namespace

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    if (argc != 3)
    {
        err() << "Usage: mklogolib <compiler-dir> <output-db>\n";
        return 1;
    }

    const QString compilerDirPath = QString::fromLocal8Bit(argv[1]);
    const QString destPath = QString::fromLocal8Bit(argv[2]);

    QFile::remove(destPath);

    if (!QSqlDatabase::isDriverAvailable(QStringLiteral("QSQLITE")))
    {
        err() << "QSQLITE driver is not available\n";
        return 1;
    }

    QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), QStringLiteral("mklogolib"));
    db.setDatabaseName(destPath);
    if (!db.open())
    {
        err() << "Cannot open database '" << destPath << "': " << db.lastError().text() << '\n';
        return 1;
    }

    QSqlQuery query(db);
    if (!query.exec(QStringLiteral("CREATE TABLE LIBRARY "
                                   "(COMMAND TEXT PRIMARY KEY NOT NULL, "
                                   "CODE TEXT NOT NULL)")))
    {
        err() << "CREATE TABLE failed: " << query.lastError().text() << '\n';
        return 1;
    }

    if (!query.prepare(QStringLiteral("INSERT INTO LIBRARY (COMMAND, CODE) VALUES (?, ?)")))
    {
        err() << "Failed to prepare INSERT: " << query.lastError().text() << '\n';
        return 1;
    }

    if (!db.transaction())
    {
        err() << "Failed to start transaction: " << db.lastError().text() << '\n';
        return 1;
    }

    if (!processCompilerDir(compilerDirPath, query))
    {
        db.rollback();
        return 1;
    }

    if (!db.commit())
    {
        err() << "Failed to commit: " << db.lastError().text() << '\n';
        return 1;
    }

    db.close();
    return 0;
}
