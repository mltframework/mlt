/*
 * kdenlivetitle_regress.cpp -- A/B visual regression harness for the
 * kdenlivetitle PlainTextItem renderer.
 *
 * The same source is linked twice: once against the baseline
 * kdenlivetitle_wrapper.cpp (extracted from a git revision at configure time)
 * and once against the working-tree wrapper. Both binaries render the same
 * title XML cases, then "compare" mode diffs the two output directories.
 *
 * Usage:
 *   kdenlivetitle_regress render  <outdir>
 *   kdenlivetitle_regress compare <baselineDir> <newDir> <reportDir>
 *
 * Copyright (c) 2026 Meltytech, LLC
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 */

#include "kdenlivetitle_wrapper.h"

#include <QApplication>
#include <QDir>
#include <QFile>
#include <QGraphicsScene>
#include <QImage>
#include <QPainter>
#include <QTextStream>

#include <cstdio>
#include <cstring>

void loadFromXml(producer_ktitle self,
                 QGraphicsScene *scene,
                 const char *templateXml,
                 const char *templateText);

#include <QPainterPath>
#ifdef KDT_NEW
void appendShapedText(QPainterPath &path,
                      const QPointF &pos,
                      const QFont &font,
                      const QString &text);
#endif

namespace {

const int kWidth = 1280;
const int kHeight = 360;

struct Case
{
    const char *name;
    QString attrs; // extra <content> attributes
    QString text;
};

QString esc(const QString &s)
{
    return s.toHtmlEscaped();
}

QString titleXml(const Case &c)
{
    return QStringLiteral(
               "<kdenlivetitle width=\"%1\" height=\"%2\" LC_NUMERIC=\"C\">"
               "<item type=\"QGraphicsTextItem\" z-index=\"1\">"
               "<position x=\"40\" y=\"40\"><transform>1,0,0,0,1,0,0,0,1</transform></position>"
               "<content box-width=\"1200\" box-height=\"280\" font-color=\"255,255,255,255\" "
               "font-outline-color=\"0,0,0,255\" font-weight=\"400\" %3>%4</content>"
               "</item></kdenlivetitle>")
        .arg(kWidth)
        .arg(kHeight)
        .arg(c.attrs, esc(c.text));
}

QList<Case> cases()
{
    const QString f = QStringLiteral("font=\"Segoe UI\" font-pixel-size=\"48\" ");
    const QString latin = QStringLiteral("Kdenlive Title Quick Brown Fox 123");
    QList<Case> l;
    // Positioning / alignment
    l << Case{"latin_left", f + "alignment=\"1\"", latin};
    l << Case{"latin_center", f + "alignment=\"4\"", latin};
    l << Case{"latin_right", f + "alignment=\"2\"", latin};
    l << Case{"multiline_left",
              f + "alignment=\"1\" line-spacing=\"0\"",
              "First line\nSecond, longer line\nThird"};
    l << Case{"multiline_center_spacing",
              f + "alignment=\"4\" line-spacing=\"20\"",
              "First line\nSecond, longer line\nThird"};
    l << Case{"multiline_right", f + "alignment=\"2\"", "First line\nSecond, longer line\nThird"};
    l << Case{"size_small_24", "font=\"Segoe UI\" font-pixel-size=\"24\" alignment=\"1\"", latin};
    l << Case{"size_large_96",
              "font=\"Segoe UI\" font-pixel-size=\"96\" alignment=\"1\"",
              "Large Ag"};
    // Decorations
    l << Case{"underline", f + "font-underline=\"1\"", latin};
    l << Case{"underline_multiline_center",
              f + "font-underline=\"1\" alignment=\"4\"",
              "Underlined\nTwo lines"};
    l << Case{"bold_legacy_flag", f + "font-bold=\"1\"", latin};
    // Fill / outline / shadow
    l << Case{"gradient", f + "gradient=\"#ff0000;#0000ff;0;100;90\"", latin};
    l << Case{"gradient_angle_0", f + "gradient=\"#ffff00;#00ff00;0;100;0\"", latin};
    l << Case{"outline_3", f + "font-outline=\"3\"", latin};
    l << Case{"outline_8", f + "font-outline=\"8\"", latin};
    l << Case{"shadow_blur0_offset0", f + "shadow=\"1;#ff000000;0;0;0\"", latin};
    l << Case{"shadow_blur4_offset6", f + "shadow=\"1;#c0000000;4;6;6\"", latin};
    l << Case{"shadow_negative_offset", f + "shadow=\"1;#c0ff0000;2;-8;-8\"", latin};
    l << Case{"shadow_outline", f + "font-outline=\"4\" shadow=\"1;#c0000000;3;5;5\"", latin};
    // Tabs
    l << Case{"tabs_200", f + "tab-width=\"200\"", "A\tB\tC\tD"};
    l << Case{"tabs_wide_words", f + "tab-width=\"180\"", "Name\tValue\tX"};
    l << Case{"tabs_leading", f + "tab-width=\"150\"", "\tIndented\tcol"};
    // Italic / overhang
    l << Case{"italic_overhang", f + "font-italic=\"1\"", "fjord ffi jjj Wavy"};
    l << Case{"italic_shadow_outline",
              f + "font-italic=\"1\" font-outline=\"3\" shadow=\"1;#c0000000;3;4;4\"",
              "fjord ffi jjj Wavy"};
    // Font fallback (Latin font, non-Latin text)
    l << Case{"fallback_cjk_in_latin_font", f, QString::fromUtf8("Mixed 中文 日本語 한국어 text")};
    // Complex scripts
    l << Case{"sinhala",
              f,
              QString::fromUtf8("ශ්‍රී ලංකා ප්‍රජාතාන්ත්‍රික "
                                "ක්‍රීඩා")};
    l << Case{"sinhala_iskoola",
              "font=\"Iskoola Pota\" font-pixel-size=\"48\"",
              QString::fromUtf8(
                  "ශ්‍රී ලංකා ප්‍රජාතාන්ත්‍රික")};
    l << Case{"devanagari", f, QString::fromUtf8("नमस्ते क्षत्रिय श्री द्वार हिन्दी")};
    l << Case{"devanagari_nirmala",
              "font=\"Nirmala UI\" font-pixel-size=\"48\"",
              QString::fromUtf8("नमस्ते क्षत्रिय श्री द्वार")};
    l << Case{"bengali", f, QString::fromUtf8("বাংলা ভাষা ক্ষমা স্বপ্ন")};
    l << Case{"tamil", f, QString::fromUtf8("தமிழ் மொழி க்ஷ ஸ்ரீ")};
    l << Case{"telugu", f, QString::fromUtf8("తెలుగు భాష క్ష్మ శ్రీ")};
    l << Case{"malayalam", f, QString::fromUtf8("മലയാളം ക്ഷ ന്റ ശ്രീ")};
    l << Case{"arabic", f, QString::fromUtf8("مرحبا بالعالم، لا إله")};
    l << Case{"arabic_right_aligned", f + "alignment=\"2\"", QString::fromUtf8("مرحبا بالعالم")};
    l << Case{"urdu",
              "font=\"Segoe UI\" font-pixel-size=\"48\"",
              QString::fromUtf8("اردو زبان، پاکستان")};
    l << Case{"chinese_simplified", f, QString::fromUtf8("简体中文，标点符号。")};
    l << Case{"chinese_traditional", f, QString::fromUtf8("繁體中文，標點符號。")};
    l << Case{"japanese", f, QString::fromUtf8("日本語のひらがなとカタカナ。")};
    l << Case{"korean", f, QString::fromUtf8("한국어 한글 문장입니다.")};
    l << Case{"mixed_ltr_rtl", f, QString::fromUtf8("Kdenlive مرحبا 2026")};
    // Combinations
    l << Case{"combo_sinhala_multiline_center",
              f
                  + "alignment=\"4\" font-outline=\"3\" gradient=\"#ffcc00;#ff0066;0;100;90\" "
                    "shadow=\"1;#c0000000;3;5;5\" font-underline=\"1\"",
              QString::fromUtf8("ශ්‍රී ලංකා\nKdenlive")};
    l << Case{"combo_devanagari_tabs_right",
              f + "alignment=\"2\" tab-width=\"220\" font-outline=\"2\"",
              QString::fromUtf8("नमस्ते\tश्री\nक्ष\tद्व")};
    return l;
}

int render(const QString &outDir)
{
    QDir().mkpath(outDir);
    int n = 0;
    for (const Case &c : cases()) {
        QGraphicsScene scene;
        struct producer_ktitle_s self;
        memset(&self, 0, sizeof(self));
        mlt_producer_init(&self.parent, nullptr);
        const QByteArray xml = titleXml(c).toUtf8();
        loadFromXml(&self, &scene, xml.constData(), nullptr);

        QImage img(kWidth, kHeight, QImage::Format_ARGB32_Premultiplied);
        img.fill(QColor(48, 52, 64));
        QPainter p(&img);
        p.setRenderHint(QPainter::Antialiasing, true);
        p.setRenderHint(QPainter::TextAntialiasing, true);
        scene.render(&p, QRectF(0, 0, kWidth, kHeight), QRectF(0, 0, kWidth, kHeight));
        p.end();
        img.save(outDir + QLatin1Char('/') + QLatin1String(c.name) + QStringLiteral(".png"));
        mlt_producer_close(&self.parent);
        ++n;
    }
    fprintf(stdout, "rendered %d cases into %s\n", n, qPrintable(outDir));
    return 0;
}

QRect inkBounds(const QImage &img, const QRgb bg)
{
    int l = img.width(), t = img.height(), r = -1, b = -1;
    for (int y = 0; y < img.height(); ++y) {
        const QRgb *line = reinterpret_cast<const QRgb *>(img.constScanLine(y));
        for (int x = 0; x < img.width(); ++x) {
            if (line[x] != bg) {
                l = qMin(l, x);
                r = qMax(r, x);
                t = qMin(t, y);
                b = qMax(b, y);
            }
        }
    }
    return r < 0 ? QRect() : QRect(QPoint(l, t), QPoint(r, b));
}

QString rectStr(const QRect &r)
{
    return r.isNull() ? QStringLiteral("empty")
                      : QStringLiteral("%1,%2 %3x%4")
                            .arg(r.left())
                            .arg(r.top())
                            .arg(r.width())
                            .arg(r.height());
}

int compare(const QString &aDir, const QString &bDir, const QString &reportDir)
{
    QDir().mkpath(reportDir);
    QFile md(reportDir + QStringLiteral("/report.md"));
    if (!md.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return 1;
    }
    QTextStream out(&md);
    out << "| Case | Changed px % | Baseline ink (x,y wxh) | New ink (x,y wxh) | dLeft | dTop | dW "
           "| dH |\n";
    out << "|---|---:|---|---|---:|---:|---:|---:|\n";

    const QRgb bg = QColor(48, 52, 64).rgb();
    for (const Case &c : cases()) {
        const QString file = QLatin1String(c.name) + QStringLiteral(".png");
        QImage a(aDir + QLatin1Char('/') + file), b(bDir + QLatin1Char('/') + file);
        if (a.isNull() || b.isNull()) {
            out << "| " << c.name << " | missing | | | | | | |\n";
            continue;
        }
        a = a.convertToFormat(QImage::Format_ARGB32);
        b = b.convertToFormat(QImage::Format_ARGB32);

        QImage diff(a.size(), QImage::Format_ARGB32);
        qint64 changed = 0;
        for (int y = 0; y < a.height(); ++y) {
            const QRgb *la = reinterpret_cast<const QRgb *>(a.constScanLine(y));
            const QRgb *lb = reinterpret_cast<const QRgb *>(b.constScanLine(y));
            QRgb *ld = reinterpret_cast<QRgb *>(diff.scanLine(y));
            for (int x = 0; x < a.width(); ++x) {
                const int d = qAbs(qRed(la[x]) - qRed(lb[x])) + qAbs(qGreen(la[x]) - qGreen(lb[x]))
                              + qAbs(qBlue(la[x]) - qBlue(lb[x]));
                if (d > 48) {
                    ++changed;
                }
                // Baseline-only ink in red, new-only ink in green, overlap grey.
                const bool ia = la[x] != bg, ib = lb[x] != bg;
                ld[x] = ia && ib ? qRgb(170, 170, 170)
                        : ia     ? qRgb(255, 60, 60)
                        : ib     ? qRgb(60, 255, 90)
                                 : qRgb(20, 20, 24);
            }
        }

        // Stacked sheet: baseline / new / overlay diff.
        QImage sheet(a.width(), a.height() * 3 + 60, QImage::Format_ARGB32);
        sheet.fill(QColor(12, 12, 14));
        QPainter p(&sheet);
        p.setPen(Qt::white);
        p.setFont(QFont(QStringLiteral("Segoe UI"), 11));
        p.drawText(8, 16, QStringLiteral("%1 — BASELINE (2ac487b5)").arg(QLatin1String(c.name)));
        p.drawImage(0, 20, a);
        p.drawText(8, a.height() + 36, QStringLiteral("NEW (working tree)"));
        p.drawImage(0, a.height() + 40, b);
        p.drawText(8,
                   2 * a.height() + 56,
                   QStringLiteral("OVERLAY: red = baseline only, green = new only, grey = both"));
        p.drawImage(0, 2 * a.height() + 60, diff);
        p.end();
        sheet.save(reportDir + QLatin1Char('/') + file);

        const QRect ra = inkBounds(a, bg), rb = inkBounds(b, bg);
        const double pct = 100.0 * changed / (a.width() * a.height());
        out << "| " << c.name << " | " << QString::number(pct, 'f', 2) << " | " << rectStr(ra)
            << " | " << rectStr(rb) << " | " << (rb.left() - ra.left()) << " | "
            << (rb.top() - ra.top()) << " | " << (rb.width() - ra.width()) << " | "
            << (rb.height() - ra.height()) << " |\n";
    }
    md.close();
    fprintf(stdout, "report written to %s/report.md\n", qPrintable(reportDir));
    return 0;
}

} // namespace

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
#ifdef KDT_NEW
    if (argc >= 2 && !strcmp(argv[1], "measure")) {
        QFont font(QStringLiteral("Segoe UI"));
        font.setPixelSize(48);
        font.setWeight(QFont::Weight(400));
        font.setHintingPreference(QFont::PreferFullHinting);
        const QString t = QStringLiteral("Kdenlive Title Quick Brown Fox 123");
        QPainterPath a;
        a.addText(0, 50, font, t);
        QPainterPath b;
        appendShapedText(b, QPointF(0, 50), font, t);
        const QRectF ra = a.boundingRect(), rb = b.boundingRect();
        fprintf(stdout,
                "addText  %f %f %f %f\nshaped   %f %f %f %f\n",
                ra.x(),
                ra.y(),
                ra.width(),
                ra.height(),
                rb.x(),
                rb.y(),
                rb.width(),
                rb.height());
        QRectF ca = a.controlPointRect(), cb = b.controlPointRect();
        fprintf(stdout, "ctl addText %f %f  shaped %f %f\n", ca.x(), ca.right(), cb.x(), cb.right());
        return 0;
    }
#endif
    if (argc >= 3 && !strcmp(argv[1], "render")) {
        return render(QString::fromLocal8Bit(argv[2]));
    }
    if (argc >= 5 && !strcmp(argv[1], "compare")) {
        return compare(QString::fromLocal8Bit(argv[2]),
                       QString::fromLocal8Bit(argv[3]),
                       QString::fromLocal8Bit(argv[4]));
    }
    fprintf(stderr,
            "usage: %s render <outdir>\n       %s compare <baselineDir> <newDir> <reportDir>\n",
            argv[0],
            argv[0]);
    return 2;
}
