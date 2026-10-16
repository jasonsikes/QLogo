//===-- util/mklogohelp.cpp - Build the Logo help SQLite database -*- C++ -*-===//
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
/// Host tool that packs help text into qlogo_help.db.
///
/// Usage: mklogohelp <compiler-dir> <output-db>
///
/// Help is taken from /***DOC ... COD***/ blocks in the compiler .cpp files.
/// The first name after /***DOC is the HELPTEXT key; every name on that line
/// (including the first) is an ALIASES entry. ALIASES.CATEGORY is the topic
/// of the source file.
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
#include <QStringList>
#include <QTextStream>
#include <cstdio>

namespace
{

QTextStream &err()
{
    static QTextStream stream(stderr);
    return stream;
}

/// Topic stored in ALIASES.CATEGORY for help text taken from \a fileName.
QString categoryForCompilerFile(const QString &fileName)
{
    if (fileName == QLatin1String("compiler_arithmetic.cpp"))
    {
        return QStringLiteral("Arithmetic");
    }
    if (fileName == QLatin1String("compiler_communication.cpp"))
    {
        return QStringLiteral("Communication");
    }
    if (fileName == QLatin1String("compiler_controlstructures.cpp"))
    {
        return QStringLiteral("Control Structures");
    }
    if (fileName == QLatin1String("compiler_datastructureprimitives.cpp"))
    {
        return QStringLiteral("Data Structure Primitives");
    }
    if (fileName == QLatin1String("compiler_graphics.cpp"))
    {
        return QStringLiteral("Graphics");
    }
    if (fileName == QLatin1String("compiler_workspacemanagement.cpp"))
    {
        return QStringLiteral("Workspace Management");
    }
    if (fileName == QLatin1String("compiler.cpp"))
    {
        return QStringLiteral("Special Variables");
    }
    return {};
}

bool insertEntry(QSqlQuery &aliasQuery,
                 QSqlQuery &textQuery,
                 const QStringList &names,
                 const QString &description,
                 const QString &category)
{
    if (names.isEmpty())
    {
        err() << "Help entry has no command names\n";
        return false;
    }

    const QString &command = names.constFirst();
    for (const QString &alias : names)
    {
        aliasQuery.bindValue(0, alias);
        aliasQuery.bindValue(1, command);
        aliasQuery.bindValue(2, category);
        if (!aliasQuery.exec())
        {
            err() << "INSERT ALIASES failed for '" << alias << "': " << aliasQuery.lastError().text() << '\n';
            return false;
        }
    }

    textQuery.bindValue(0, command);
    textQuery.bindValue(1, description);
    if (!textQuery.exec())
    {
        err() << "INSERT HELPTEXT failed for '" << command << "': " << textQuery.lastError().text() << '\n';
        return false;
    }
    return true;
}

bool processCompilerFile(const QFileInfo &info, QSqlQuery &aliasQuery, QSqlQuery &textQuery)
{
    const QString category = categoryForCompilerFile(info.fileName());

    QFile file(info.absoluteFilePath());
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        err() << "Cannot read '" << info.absoluteFilePath() << "': " << file.errorString() << '\n';
        return false;
    }

    QTextStream in(&file);

    while (!in.atEnd())
    {
        const QString line = in.readLine();
        if (!line.trimmed().startsWith(QLatin1String("/***DOC")))
            continue;

        const QStringList words = line.simplified().split(QLatin1Char(' '));
        if (words.size() < 2)
        {
            err() << "Malformed /***DOC header in '" << info.fileName() << "'\n";
            return false;
        }
        const QStringList names = words.mid(1);
        if (category.isEmpty())
        {
            err() << "No help category for '" << info.fileName() << "'\n";
            return false;
        }

        QString description;
        bool foundEnd = false;
        while (!in.atEnd())
        {
            const QString textLine = in.readLine();
            if (textLine.trimmed().startsWith(QLatin1String("COD***/")))
            {
                foundEnd = true;
                break;
            }
            description += textLine;
            description += QLatin1Char('\n');
        }
        if (!foundEnd)
        {
            err() << "Unterminated /***DOC block in '" << info.fileName() << "'\n";
            return false;
        }

        if (!insertEntry(aliasQuery, textQuery, names, description, category))
            return false;
    }

    return true;
}

bool processCompilerDir(const QString &sourceDirPath, QSqlQuery &aliasQuery, QSqlQuery &textQuery)
{
    QDir sourceDir(sourceDirPath);
    if (!sourceDir.exists())
    {
        err() << "Compiler directory does not exist: " << sourceDirPath << '\n';
        return false;
    }

    const QFileInfoList files = sourceDir.entryInfoList(QStringList{QStringLiteral("*.cpp")},
                                                        QDir::Files | QDir::Readable | QDir::NoDotAndDotDot,
                                                        QDir::Name);
    if (files.isEmpty())
    {
        err() << "No .cpp files found in " << sourceDirPath << '\n';
        return false;
    }

    for (const QFileInfo &info : files)
    {
        if (!processCompilerFile(info, aliasQuery, textQuery))
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
        err() << "Usage: mklogohelp <compiler-dir> <output-db>\n";
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

    QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), QStringLiteral("mklogohelp"));
    db.setDatabaseName(destPath);
    if (!db.open())
    {
        err() << "Cannot open database '" << destPath << "': " << db.lastError().text() << '\n';
        return 1;
    }

    QSqlQuery query(db);
    if (!query.exec(QStringLiteral("CREATE TABLE ALIASES "
                                   "(ALIAS TEXT PRIMARY KEY NOT NULL, "
                                   "COMMAND TEXT NOT NULL, "
                                   "CATEGORY TEXT NOT NULL)")) ||
        !query.exec(QStringLiteral("CREATE TABLE HELPTEXT "
                                   "(COMMAND TEXT PRIMARY KEY NOT NULL, "
                                   "DESCRIPTION TEXT)")))
    {
        err() << "CREATE TABLE failed: " << query.lastError().text() << '\n';
        return 1;
    }

    QSqlQuery aliasQuery(db);
    QSqlQuery textQuery(db);
    if (!aliasQuery.prepare(
            QStringLiteral("INSERT INTO ALIASES (ALIAS, COMMAND, CATEGORY) VALUES (?, ?, ?)")) ||
        !textQuery.prepare(QStringLiteral("INSERT INTO HELPTEXT (COMMAND, DESCRIPTION) VALUES (?, ?)")))
    {
        err() << "Failed to prepare INSERT: " << db.lastError().text() << '\n';
        return 1;
    }

    if (!db.transaction())
    {
        err() << "Failed to start transaction: " << db.lastError().text() << '\n';
        return 1;
    }

    if (!processCompilerDir(compilerDirPath, aliasQuery, textQuery))
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
