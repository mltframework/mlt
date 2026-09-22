// SPDX-License-Identifier: MIT
#pragma once

#include <algorithm>
#include <cmath>
#include <random>
#include <QAbstractTextDocumentLayout>
#include <QGraphicsTextItem>
#include <QPainter>
#include <QTextBoundaryFinder>
#include <QTextCursor>
#include <QTextDocument>
#include <QVector>

// Rich text: immutable text plus a frame-to-visible-range schedule.
// No parser metacharacters are interpreted in the three automatic modes.
namespace RichTextReveal {
class Schedule
{
public:
    bool reset(const QString &text, int step, int mode, int sigma, unsigned int seed)
    {
        m_steps.clear();
        m_text = text;
        if (step < 1 || mode < 1 || mode > 3 || sigma < 0) {
            return false;
        }
        QVector<int> ends;
        if (mode == 1) {
            QTextBoundaryFinder finder(QTextBoundaryFinder::Grapheme, text);
            for (int end = finder.toNextBoundary(); end >= 0; end = finder.toNextBoundary()) {
                if (end > 0)
                    ends.append(end);
            }
        } else {
            // Preserve the legacy automatic word/line grouping, including
            // whitespace after the word and consecutive empty lines.
            int pos = 0;
            while (pos < text.size()) {
                auto separator = [mode](QChar c) {
                    return mode == 2 ? c.isSpace() : c == QLatin1Char('\n');
                };
                while (pos < text.size() && !separator(text.at(pos)))
                    ++pos;
                while (pos < text.size() && separator(text.at(pos)))
                    ++pos;
                ends.append(pos);
            }
        }
        std::mt19937 generator(seed);
        std::normal_distribution<double> jitter(0.0, std::max(1, sigma));
        qint64 previous = -1;
        for (int i = 0; i < ends.size(); ++i) {
            qint64 frame = qint64(i) * step;
            if (sigma > 0) {
                // Finite input and a bound before conversion prevent overflow.
                const double value = jitter(generator);
                const qint64 delta = qint64(
                    std::llround(std::max(-1.0e12, std::min(1.0e12, value))));
                if (frame + delta > 0)
                    frame += delta;
            }
            frame = std::max(previous + 1, frame);
            m_steps.append({frame, ends.at(i)});
            previous = frame;
        }
        return true;
    }

    int visible(qint64 frame) const
    {
        const auto it = std::upper_bound(m_steps.cbegin(),
                                         m_steps.cend(),
                                         frame,
                                         [](qint64 value, const Step &s) {
                                             return value < s.frame;
                                         });
        return it == m_steps.cbegin() ? 0 : (it - 1)->end;
    }
    const QString &text() const { return m_text; }

private:
    struct Step
    {
        qint64 frame;
        int end;
    };
    QString m_text;
    QVector<Step> m_steps;
};

// Keep the complete final layout. Removing the suffix would shift centered
// text, change wrapping, and change line heights as larger fonts appear.
class Item : public QGraphicsTextItem
{
public:
    void configure(const QStringList &parameters, bool externallyDriven)
    {
        m_total = document()->characterCount() - 1;
        m_text = document()->toPlainText();
        m_visible = m_total;
        m_native = false;
        if (!externallyDriven && parameters.size() >= 5 && parameters.at(0).toInt() != 0) {
            m_native = m_schedule.reset(document()->toPlainText(),
                                        parameters.at(1).toInt(),
                                        parameters.at(2).toInt(),
                                        parameters.at(3).toInt(),
                                        parameters.at(4).toUInt());
        }
        document()->setUndoRedoEnabled(false);
    }
    bool animated() const { return m_native; }
    void setFrame(qint64 frame)
    {
        if (m_native)
            setVisibleCharacters(m_schedule.visible(frame));
    }
    void setVisibleCharacters(int count)
    {
        count = std::max(0, std::min(m_total, count));
        // Never display half a surrogate, combining sequence, or emoji cluster.
        if (count == m_visible)
            return;
        QTextBoundaryFinder finder(QTextBoundaryFinder::Grapheme, m_text);
        finder.setPosition(count);
        if (!finder.isAtBoundary())
            count = int(std::max<qsizetype>(0, finder.toPreviousBoundary()));
        if (count != m_visible) {
            m_visible = count;
            update(); // Also invalidates the attached shadow effect's source.
        }
    }

protected:
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *) override
    {
        QAbstractTextDocumentLayout::PaintContext context;
        context.palette.setColor(QPalette::Text, defaultTextColor());
        if (m_visible < m_total) {
            QAbstractTextDocumentLayout::Selection hidden;
            hidden.cursor = QTextCursor(document());
            hidden.cursor.setPosition(m_visible);
            hidden.cursor.setPosition(m_total, QTextCursor::KeepAnchor);
            hidden.format.setForeground(QBrush(Qt::transparent));
            hidden.format.setBackground(QBrush(Qt::transparent));
            // A transparent SOLID outline takes Qt's vector paint path for
            // the hidden range; NoPen can still render bitmap/color emoji.
            // This is a paint-context override, not a document format change.
            hidden.format.setTextOutline(QPen(QBrush(Qt::transparent), 0.0, Qt::SolidLine));
            hidden.format.setFontUnderline(false);
            hidden.format.setFontStrikeOut(false);
            context.selections.append(hidden);
        }
        painter->save();
        document()->documentLayout()->draw(painter, context);
        painter->restore();
    }

private:
    Schedule m_schedule;
    QString m_text;
    int m_total{0};
    int m_visible{0};
    bool m_native{false};
};
} // namespace RichTextReveal
