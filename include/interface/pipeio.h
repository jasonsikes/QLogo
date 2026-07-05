#ifndef PIPEIO_H
#define PIPEIO_H

//===-- qlogo/pipeio.h - stdio pipe I/O portability -------*- C++ -*-===//
//
// Copyright 2017-2024 Jason Sikes
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted under the conditions specified in the
// license found in the LICENSE file in the project root.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// Portable read/write helpers for the Psychi <-> qlogo binary pipe protocol.
///
//===----------------------------------------------------------------------===//

#ifdef _WIN32
#include <cstdio>
#include <fcntl.h>
#include <io.h>
#else
#include <unistd.h>
#endif

#ifdef _WIN32
inline int pipeRead(int fd, void *buf, size_t count)
{
    return _read(fd, static_cast<char *>(buf), static_cast<unsigned int>(count));
}

inline int pipeWrite(int fd, const void *buf, size_t count)
{
    return _write(fd, static_cast<const char *>(buf), static_cast<unsigned int>(count));
}

inline void setStdioBinaryMode()
{
    _setmode(_fileno(stdin), _O_BINARY);
    _setmode(_fileno(stdout), _O_BINARY);
}

inline int stdinFd()
{
    return _fileno(stdin);
}

inline int stdoutFd()
{
    return _fileno(stdout);
}
#else
inline ssize_t pipeRead(int fd, void *buf, size_t count)
{
    return read(fd, buf, count);
}

inline ssize_t pipeWrite(int fd, const void *buf, size_t count)
{
    return write(fd, buf, count);
}

inline void setStdioBinaryMode() {}

inline int stdinFd()
{
    return STDIN_FILENO;
}

inline int stdoutFd()
{
    return STDOUT_FILENO;
}
#endif

#endif // PIPEIO_H
