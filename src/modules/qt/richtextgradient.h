// SPDX-License-Identifier: MIT
#pragma once

#include <QBrush>
#include <QCryptographicHash>
#include <QDomDocument>
#include <QLinearGradient>
#include <QTextBlock>
#include <QTextCharFormat>
#include <QTextCursor>
#include <QTextDocument>
#include <QTextFormat>
#include <QTextFragment>
#include <QVector>
#include <QtMath>

// Rich text: Qt HTML does not serialize gradient brushes. Keep the gradient
// recipe as a custom QTextCharFormat property and persist only the ranges that
// use it. Positions and lengths are QTextCursor UTF-16 units.
namespace TitlerGradientV1 {
constexpr int Property = QTextFormat::UserProperty + 1;

inline QString textHash(const QTextDocument *text)
{
    return QString::fromLatin1(
        QCryptographicHash::hash(text->toPlainText().toUtf8(), QCryptographicHash::Sha256).toHex());
}

inline QLinearGradient gradientFromString(const QString &data, int width, int height)
{
    const QStringList values = data.split(QLatin1Char(';'));
    QLinearGradient gradient;
    if (values.size() < 5) {
        return gradient;
    }
    gradient.setColorAt(values.at(2).toDouble() / 100.0, QColor(values.at(0)));
    gradient.setColorAt(values.at(3).toDouble() / 100.0, QColor(values.at(1)));
    const double angle = values.at(4).toDouble();
    if (angle <= 90.0) {
        gradient.setStart(0, 0);
        gradient.setFinalStop(width * qCos(qDegreesToRadians(angle)),
                              height * qSin(qDegreesToRadians(angle)));
    } else {
        gradient.setStart(width, 0);
        gradient.setFinalStop(width + width * qCos(qDegreesToRadians(angle)),
                              height * qSin(qDegreesToRadians(angle)));
    }
    return gradient;
}

inline QDomElement save(QDomDocument &xml, const QTextDocument *text)
{
    QDomElement result = xml.createElement(QStringLiteral("richtext-gradients"));
    result.setAttribute(QStringLiteral("version"), 1);
    result.setAttribute(QStringLiteral("units"), QStringLiteral("utf16"));
    result.setAttribute(QStringLiteral("characters"), text->characterCount() - 1);
    result.setAttribute(QStringLiteral("text-sha256"), textHash(text));
    for (QTextBlock block = text->begin(); block.isValid(); block = block.next()) {
        for (auto it = block.begin(); !it.atEnd(); ++it) {
            const QTextFragment fragment = it.fragment();
            if (!fragment.isValid())
                continue;
            const QString data = fragment.charFormat().property(Property).toString();
            if (data.isEmpty())
                continue;
            QDomElement run = xml.createElement(QStringLiteral("run"));
            run.setAttribute(QStringLiteral("start"), fragment.position());
            run.setAttribute(QStringLiteral("length"), fragment.length());
            run.setAttribute(QStringLiteral("data"), data);
            result.appendChild(run);
        }
    }
    return result;
}

inline bool restore(const QDomElement &content, QTextDocument *text)
{
    const QDomElement data = content.firstChildElement(QStringLiteral("richtext-gradients"));
    if (data.isNull())
        return true;
    bool sizeOk = false;
    const int total = text->characterCount() - 1;
    const int savedSize = data.attribute(QStringLiteral("characters")).toInt(&sizeOk);
    if (data.attribute(QStringLiteral("version")) != QLatin1String("1")
        || data.attribute(QStringLiteral("units")) != QLatin1String("utf16") || !sizeOk
        || savedSize != total || data.attribute(QStringLiteral("text-sha256")) != textHash(text)
        || !data.nextSiblingElement(QStringLiteral("richtext-gradients")).isNull()) {
        return false;
    }
    struct Run
    {
        int start;
        int length;
        QString data;
    };
    QVector<Run> runs;
    const QString plain = text->toPlainText();
    if (plain.size() != total)
        return false;
    const auto splitsSurrogate = [&plain, total](int position) {
        return position > 0 && position < total && plain.at(position - 1).isHighSurrogate()
               && plain.at(position).isLowSurrogate();
    };
    int previousEnd = 0;
    for (QDomElement e = data.firstChildElement(); !e.isNull(); e = e.nextSiblingElement()) {
        bool a = false, b = false;
        const Run run{e.attribute(QStringLiteral("start")).toInt(&a),
                      e.attribute(QStringLiteral("length")).toInt(&b),
                      e.attribute(QStringLiteral("data"))};
        if (e.tagName() != QLatin1String("run") || !a || !b || run.data.isEmpty()
            || run.start < previousEnd || run.length <= 0 || run.length > total
            || run.start > total - run.length || splitsSurrogate(run.start)
            || splitsSurrogate(run.start + run.length)) {
            return false;
        }
        previousEnd = run.start + run.length;
        runs.append(run);
    }
    QTextCursor cursor(text);
    cursor.beginEditBlock();
    for (const Run &run : runs) {
        cursor.setPosition(run.start);
        cursor.setPosition(run.start + run.length, QTextCursor::KeepAnchor);
        QTextCharFormat delta;
        delta.setProperty(Property, run.data);
        cursor.mergeCharFormat(delta);
    }
    cursor.endEditBlock();
    return true;
}

inline void applyBrushes(QTextDocument *text, int width, int height)
{
    struct Run
    {
        int start;
        int length;
        QString data;
    };
    QVector<Run> runs;
    for (QTextBlock block = text->begin(); block.isValid(); block = block.next()) {
        for (auto it = block.begin(); !it.atEnd(); ++it) {
            const QTextFragment fragment = it.fragment();
            if (!fragment.isValid())
                continue;
            const QString data = fragment.charFormat().property(Property).toString();
            if (!data.isEmpty())
                runs.append({fragment.position(), fragment.length(), data});
        }
    }
    QTextCursor cursor(text);
    cursor.beginEditBlock();
    for (const Run &run : runs) {
        const QBrush brush(gradientFromString(run.data, width, height));
        cursor.setPosition(run.start);
        cursor.setPosition(run.start + run.length, QTextCursor::KeepAnchor);
        if (cursor.charFormat().foreground() != brush) {
            QTextCharFormat delta;
            delta.setForeground(brush);
            cursor.mergeCharFormat(delta);
        }
    }
    cursor.endEditBlock();
}
} // namespace TitlerGradientV1
