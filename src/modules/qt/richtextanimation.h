// SPDX-License-Identifier: MIT
#pragma once

#include "richtextoutline.h"

#include <algorithm>
#include <cmath>
#include <random>
#include <QAbstractTextDocumentLayout>
#include <QGraphicsTextItem>
#include <QPainter>
#include <QPen>
#include <QGlyphRun>
#include <QFontMetricsF>
#include <QRawFont>
#include <QTextBlock>
#include <QTextFragment>
#include <QTextLayout>
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
    void setOutline(qreal width, const QColor &color)
    {
        width = std::isfinite(width) ? std::max(qreal(0), width) : qreal(0);
        prepareGeometryChange();
        m_outlinePen = QPen(color.isValid() ? color : QColor(Qt::black), width,
                            width > 0 ? Qt::SolidLine : Qt::NoPen, Qt::RoundCap, Qt::RoundJoin);
        QTextCursor cursor(document());
        cursor.select(QTextCursor::Document);
        QTextCharFormat fillOnly;
        fillOnly.setTextOutline(QPen(Qt::NoPen));
        cursor.mergeCharFormat(fillOnly);
        m_outlineMargin = TitlerOutline::margin(document(), m_outlinePen);
    }
    QRectF boundingRect() const override
    {
        const qreal margin = m_outlineMargin;
        return QGraphicsTextItem::boundingRect().united(m_inkBounds).adjusted(-margin, -margin, margin, margin);
    }
    void configure(const QStringList &parameters, bool externallyDriven)
    {
        prepareGeometryChange();
        m_inkBounds = TitlerOutline::inkBounds(document());
        m_outlineMargin = TitlerOutline::margin(document(), m_outlinePen);
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
    struct VisualCell {
        QRectF rect;
        QRectF visibleRect;
        qreal baseline;
        int lineId;
        QTextCharFormat format;
        QRawFont physicalFont;
    };

    QVector<VisualCell> visibleCells() const
    {
        QVector<VisualCell> cells;
        // Collect the final line, including the hidden suffix: a later fallback
        // font must not change the underline metrics of already visible text.
        for (QTextBlock block = document()->begin(); block.isValid(); block = block.next()) {
            const QTextLayout *layout = block.layout();
            if (!layout) {
                continue;
            }
            const QString text = block.text();
            QTextBoundaryFinder finder(QTextBoundaryFinder::Grapheme, text);
            int start = 0;
            for (int end = finder.toNextBoundary(); end >= 0; start = end, end = finder.toNextBoundary()) {
                const QTextLine line = layout->lineForTextPosition(start);
                if (!line.isValid() || end <= start) {
                    continue;
                }
                QTextCursor cursor(document());
                cursor.setPosition(block.position() + start);
                cursor.setPosition(block.position() + end, QTextCursor::KeepAnchor);
                const QTextCharFormat format = cursor.charFormat();
                QRawFont physicalFont;
                qreal largestAdvance = -1;
                // Decorations belong to the advancing font in the shaped
                // cluster. Zero-advance fallback marks do not gain a second
                // cell-wide decoration. No shaping or positions are replaced.
                const auto runs = line.glyphRuns(start, end - start);
                for (const QGlyphRun &run : runs) {
                    const QRawFont raw = run.rawFont();
                    if (!raw.isValid()) {
                        continue;
                    }
                    qreal advance = 0;
                    for (const QPointF &a : raw.advancesForGlyphIndexes(run.glyphIndexes())) {
                        advance += std::abs(a.x());
                    }
                    if (!physicalFont.isValid() || advance > largestAdvance) {
                        physicalFont = raw;
                        largestAdvance = advance;
                    }
                }
                if (!physicalFont.isValid()) {
                    // Tabs and other advance-only cells can have no glyph run.
                    physicalFont = QRawFont::fromFont(format.font().resolve(document()->defaultFont()));
                }
                const qreal a = line.cursorToX(start);
                const qreal z = line.cursorToX(end);
                const QPointF origin = layout->position();
                const QRectF rect(origin.x() + std::min(a, z), origin.y() + line.y(),
                                  std::abs(z - a), line.height());
                cells.append({rect, block.position() + end <= m_visible ? rect : QRectF(),
                              origin.y() + line.y() + line.ascent(), block.position() + line.textStart(),
                              format, physicalFont});
            }
        }
        std::sort(cells.begin(), cells.end(), [](const VisualCell &a, const VisualCell &b) {
            if (a.lineId != b.lineId) {
                return a.lineId < b.lineId;
            }
            return a.rect.left() < b.rect.left();
        });
        QVector<VisualCell> merged;
        for (const VisualCell &cell : cells) {
            // Keep fallback-font item boundaries. Native backgrounds are
            // aligned and composited per item, not once for an entire paragraph.
            if (!merged.isEmpty() && merged.last().lineId == cell.lineId
                && merged.last().baseline == cell.baseline
                && merged.last().rect.height() == cell.rect.height()
                && merged.last().format == cell.format
                && merged.last().physicalFont == cell.physicalFont
                && std::abs(cell.rect.left() - merged.last().rect.right()) <= qreal(1.0 / 64)) {
                merged.last().rect = merged.last().rect.united(cell.rect);
                if (!cell.visibleRect.isEmpty()) {
                    merged.last().visibleRect = merged.last().visibleRect.isEmpty()
                        ? cell.visibleRect : merged.last().visibleRect.united(cell.visibleRect);
                }
            } else {
                merged.append(cell);
            }
        }
        return merged;
    }

    void paintBackgrounds(QPainter *painter, const QVector<VisualCell> &cells) const
    {
        painter->save();
        for (const VisualCell &cell : cells) {
            const QBrush brush = cell.format.background();
            if (brush.style() != Qt::NoBrush && !cell.visibleRect.isEmpty()) {
                painter->fillRect(cell.visibleRect.toAlignedRect(), brush);
            }
        }
        painter->restore();
    }

    void paintDecorations(QPainter *painter, const QVector<VisualCell> &cells) const
    {
        struct Decoration {
            int lineId;
            qreal fullLeft;
            qreal fullRight;
            qreal left;
            qreal right;
            qreal y;
            QPen pen;
        };
        QVector<Decoration> underlines;
        QVector<Decoration> strikes;
        QVector<Decoration> overlines;
        painter->save();
        painter->setBrush(Qt::NoBrush);
        for (const VisualCell &cell : cells) {
            const QTextCharFormat &format = cell.format;
            if (!format.fontUnderline() && !format.fontOverline() && !format.fontStrikeOut()) {
                continue;
            }
            const QBrush brush = format.foreground().style() == Qt::NoBrush
                ? QBrush(defaultTextColor()) : format.foreground();
            const QFontMetricsF fallback(format.font().resolve(document()->defaultFont()), painter->device());
            const bool raw = cell.physicalFont.isValid();
            const qreal ascent = raw ? cell.physicalFont.ascent() : fallback.ascent();
            const qreal descent = raw ? cell.physicalFont.descent() : fallback.descent();
            const qreal thickness = raw ? cell.physicalFont.lineThickness() : fallback.lineWidth();
            const qreal underlinePosition = raw ? cell.physicalFont.underlinePosition() : fallback.underlinePos();
            QPen pen(brush, thickness, Qt::SolidLine, Qt::FlatCap);
            if (format.underlineColor().isValid()) {
                pen.setColor(format.underlineColor());
            }
            const bool visible = !cell.visibleRect.isEmpty()
                && !(pen.brush().style() == Qt::SolidPattern && pen.color().alpha() == 0);
            const qreal left = visible ? std::floor(cell.visibleRect.left()) : 0;
            const qreal right = visible ? std::floor(cell.visibleRect.right()) : 0;
            const qreal fullLeft = std::floor(cell.rect.left());
            const qreal fullRight = std::floor(cell.rect.right());
            if (format.fontUnderline()) {
                QPen underlinePen = pen;
                if (!painter->testRenderHint(QPainter::Antialiasing)) {
                    underlinePen.setWidthF(std::max(qreal(1), qreal(qRound(thickness))));
                }
                // A contiguous underline has common final-layout position and
                // thickness, while each segment retains its foreground color.
                qreal offset = std::ceil(underlinePosition) + underlinePen.widthF() / 2;
                if (underlinePosition <= descent) {
                    offset = std::min(offset, descent - underlinePen.widthF() / 2);
                }
                const auto style = format.underlineStyle();
                if (style >= QTextCharFormat::SingleUnderline && style <= QTextCharFormat::DashDotDotLine) {
                    underlinePen.setStyle(static_cast<Qt::PenStyle>(style));
                }
                underlines.append({cell.lineId, fullLeft, fullRight, left, right,
                                   cell.baseline + offset, underlinePen});
            }
            if (format.fontStrikeOut()) {
                strikes.append({cell.lineId, fullLeft, fullRight, left, right,
                                cell.baseline - ascent / 3, pen});
            }
            if (format.fontOverline()) {
                overlines.append({cell.lineId, fullLeft, fullRight, left, right,
                                  cell.baseline - ascent, pen});
            }
        }
        for (qsizetype start = 0; start < underlines.size();) {
            qsizetype end = start + 1;
            qreal y = underlines[start].y;
            qreal width = underlines[start].pen.widthF();
            while (end < underlines.size() && underlines[end].lineId == underlines[start].lineId
                   && underlines[end - 1].fullRight == underlines[end].fullLeft) {
                y = std::max(y, underlines[end].y);
                width = std::max(width, underlines[end].pen.widthF());
                ++end;
            }
            for (qsizetype i = start; i < end; ++i) {
                underlines[i].y = y;
                underlines[i].pen.setWidthF(width);
            }
            start = end;
        }
        const auto draw = [painter](const QVector<Decoration> &segments) {
            for (const auto &segment : segments) {
                if (segment.right > segment.left) {
                    painter->setPen(segment.pen);
                    painter->drawLine(QLineF(segment.left, segment.y, segment.right, segment.y));
                }
            }
        };
        draw(underlines);
        draw(strikes);
        draw(overlines);
        painter->restore();
    }

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *) override
    {
        if (m_visible <= 0) {
            return;
        }
        painter->save();
        TitlerOutline::paint(painter, document(), m_outlinePen, m_visible);
        {
            // The final reveal uses the same cells, glyphs and decorations as earlier frames.
            const auto cells = visibleCells();
            paintBackgrounds(painter, cells);
            // Draw glyphs at their full-layout positions; decorations use line cells.
            for (QTextBlock block = document()->begin(); block.isValid() && block.position() < m_visible; block = block.next()) {
                const QTextLayout *layout = block.layout();
                for (auto it = block.begin(); !it.atEnd(); ++it) {
                    const QTextFragment fragment = it.fragment();
                    if (!fragment.isValid() || fragment.position() >= m_visible) {
                        continue;
                    }
                    const int count = std::min(fragment.length(), m_visible - fragment.position());
                    const QTextCharFormat format = fragment.charFormat();
                    const QBrush brush = format.foreground().style() == Qt::NoBrush
                        ? QBrush(defaultTextColor()) : format.foreground();
                    const auto runs = layout->glyphRuns(fragment.position() - block.position(), count);
                    for (const QGlyphRun &run : runs) {
                        painter->save();
                        if (run.flags().testFlag(QGlyphRun::SplitLigature)) {
                            // Qt explicitly marks ranges representing only part
                            // of a shared glyph. Retain that case's own clip.
                            painter->setClipRect(run.boundingRect().translated(layout->position()), Qt::IntersectClip);
                        }
                        // A zero-alpha solid foreground must hide bitmap glyphs too.
                        if (brush.style() != Qt::SolidPattern || brush.color().alpha() != 0) {
                            painter->setPen(QPen(brush, 0));
                            QGlyphRun glyphs = run;
                            glyphs.setUnderline(false);
                            glyphs.setOverline(false);
                            glyphs.setStrikeOut(false);
                            painter->drawGlyphRun(layout->position(), glyphs);
                        }
                        painter->restore();
                    }
                }
            }
            paintDecorations(painter, cells);
        }
        painter->restore();
    }

private:
    QPen m_outlinePen{Qt::NoPen};
    qreal m_outlineMargin{0};
    QRectF m_inkBounds;
    Schedule m_schedule;
    QString m_text;
    int m_total{0};
    int m_visible{0};
    bool m_native{false};
};
} // namespace RichTextReveal
