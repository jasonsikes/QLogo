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
/// Host tool that packs logolib source files into qlogo_library.db.
///
/// Usage: mklogolib <logolib-dir> <output-db>
///
/// Each file in the directory becomes one LIBRARY row. COMMAND is the
/// filename with UCBLogo's filesystem encoding reversed (capital Q stands
/// for '?') and then converted to the Logo key form (uppercase). CODE is
/// the file contents.
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

/// Map a logolib filename to the LIBRARY.COMMAND key.
///
/// '?' is problematic in a filename, so capital Q is used instead.
/// Replace those Qs, then uppercase so the key matches Word::genKeyString().
QString commandFromFileName(QString name)
{
    if (name.endsWith(QLatin1String(".lg"), Qt::CaseInsensitive))
        name.chop(3);
    name.replace(QLatin1Char('Q'), QLatin1Char('?'));
    return name.toUpper();
}

bool insertLibraryFile(QSqlQuery &query, const QFileInfo &info)
{
    const QString command = commandFromFileName(info.fileName());
    if (command.isEmpty())
    {
        err() << "Skipping empty command name from '" << info.fileName() << "'\n";
        return true;
    }

    QFile file(info.absoluteFilePath());
    if (!file.open(QIODevice::ReadOnly))
    {
        err() << "Cannot read '" << info.absoluteFilePath() << "': " << file.errorString() << '\n';
        return false;
    }

    const QString code = QString::fromUtf8(file.readAll());

    query.addBindValue(command);
    query.addBindValue(code);
    if (!query.exec())
    {
        err() << "INSERT failed for '" << command << "': " << query.lastError().text() << '\n';
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
        err() << "Usage: mklogolib <logolib-dir> <output-db>\n";
        return 1;
    }

    const QString sourceDirPath = QString::fromLocal8Bit(argv[1]);
    const QString destPath = QString::fromLocal8Bit(argv[2]);

    QDir sourceDir(sourceDirPath);
    if (!sourceDir.exists())
    {
        err() << "Logolib directory does not exist: " << sourceDirPath << '\n';
        return 1;
    }

    const QFileInfoList files =
        sourceDir.entryInfoList(QDir::Files | QDir::Readable | QDir::NoDotAndDotDot, QDir::Name);
    if (files.isEmpty())
    {
        err() << "No files found in " << sourceDirPath << '\n';
        return 1;
    }

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

    for (const QFileInfo &info : files)
    {
        if (!insertLibraryFile(query, info))
        {
            db.rollback();
            return 1;
        }
    }

    if (!db.commit())
    {
        err() << "Failed to commit: " << db.lastError().text() << '\n';
        return 1;
    }

    db.close();
    return 0;
}
