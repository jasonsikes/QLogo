//===-- qlogo/lineeditor.cpp - LineEditor class implementation -------*- C++ -*-===//
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
/// This file contains the implementation of the LineEditor class, which holds
/// the buffer, cursor, prompt origin, and session history for a console prompt.
///
//===----------------------------------------------------------------------===//

#include "interface/lineeditor.h"

void LineEditor::clampCursor()
{
    if (cursor_ < 0)
        cursor_ = 0;
    if (cursor_ > buffer_.size())
        cursor_ = buffer_.size();
}

void LineEditor::saveScratchIfEdited()
{
    if (historyIndex_ < 0 || historyIndex_ >= history_.size())
        return;
    if (buffer_ != history_[historyIndex_])
        history_.last() = buffer_;
}

void LineEditor::loadHistoryIndex(int newIndex)
{
    historyIndex_ = newIndex;
    buffer_ = history_[historyIndex_];
    cursor_ = buffer_.size();
}

void LineEditor::beginPrompt(int startY, int startX)
{
    buffer_.resize(0);
    cursor_ = 0;
    startY_ = startY;
    startX_ = startX;
    if (!promptActive_)
    {
        history_.push_back(QString());
        historyIndex_ = history_.size() - 1;
        promptActive_ = true;
    }
    else
    {
        historyIndex_ = history_.size() - 1;
        history_.last() = QString();
    }
}

void LineEditor::setOrigin(int startY, int startX)
{
    startY_ = startY;
    startX_ = startX;
}

void LineEditor::setCursor(int index)
{
    cursor_ = index;
    clampCursor();
}

void LineEditor::moveLeft()
{
    if (cursor_ <= 0)
        return;
    --cursor_;
    if (cursor_ > 0 && buffer_[cursor_].isLowSurrogate() && buffer_[cursor_ - 1].isHighSurrogate())
        --cursor_;
}

void LineEditor::moveRight()
{
    if (cursor_ >= buffer_.size())
        return;
    if (cursor_ + 1 < buffer_.size() && buffer_[cursor_].isHighSurrogate() && buffer_[cursor_ + 1].isLowSurrogate())
        cursor_ += 2;
    else
        ++cursor_;
}

void LineEditor::moveHome()
{
    cursor_ = 0;
}

void LineEditor::moveEnd()
{
    cursor_ = buffer_.size();
}

void LineEditor::eraseLeft()
{
    const int end = cursor_;
    moveLeft();
    if (cursor_ < end)
        buffer_.remove(cursor_, end - cursor_);
}

void LineEditor::eraseAtCursor()
{
    if (cursor_ >= buffer_.size())
        return;
    const int start = cursor_;
    moveRight();
    const int len = cursor_ - start;
    cursor_ = start;
    buffer_.remove(start, len);
}

void LineEditor::killToEnd()
{
    if (cursor_ < buffer_.size())
        buffer_.truncate(cursor_);
}

void LineEditor::killLine()
{
    buffer_.resize(0);
    cursor_ = 0;
}

void LineEditor::killWord()
{
    if (cursor_ <= 0)
        return;
    const int end = cursor_;
    auto movedRangeIsSpace = [this](int from, int to) {
        for (int i = from; i < to; ++i)
        {
            if (!buffer_[i].isSpace())
                return false;
        }
        return from < to;
    };
    while (cursor_ > 0)
    {
        const int saved = cursor_;
        moveLeft();
        if (!movedRangeIsSpace(cursor_, saved))
        {
            cursor_ = saved;
            break;
        }
    }
    while (cursor_ > 0)
    {
        const int saved = cursor_;
        moveLeft();
        if (movedRangeIsSpace(cursor_, saved))
        {
            cursor_ = saved;
            break;
        }
    }
    buffer_.remove(cursor_, end - cursor_);
}

void LineEditor::insert(char32_t ch)
{
    insert(QString::fromUcs4(&ch, 1));
}

void LineEditor::insert(const QString &text)
{
    buffer_.insert(cursor_, text);
    cursor_ += text.size();
}

QString LineEditor::finish()
{
    if (promptActive_ && !history_.isEmpty())
        history_.last() = buffer_;
    promptActive_ = false;
    return buffer_;
}

bool LineEditor::recallOlder()
{
    if (!promptActive_ || historyIndex_ <= 0)
        return false;
    saveScratchIfEdited();
    loadHistoryIndex(historyIndex_ - 1);
    return true;
}

bool LineEditor::recallNewer()
{
    if (!promptActive_ || historyIndex_ >= history_.size() - 1)
        return false;
    saveScratchIfEdited();
    loadHistoryIndex(historyIndex_ + 1);
    return true;
}
