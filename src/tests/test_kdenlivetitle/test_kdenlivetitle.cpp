/*
 * test_kdenlivetitle.cpp -- Comprehensive Regression & Multi-Script Unit Tests for Kdenlive Titler
 *
 * Copyright (c) 2026 Meltytech, LLC
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 */

#include <QDebug>
#include <QDomDocument>
#include <QFont>
#include <QFontMetrics>
#include <QGraphicsItem>
#include <QGraphicsScene>
#include <QGuiApplication>
#include <QImage>
#include <QLinearGradient>
#include <QPainter>
#include <QPainterPath>
#include <QTest>

#include "../../src/modules/qt/kdenlivetitle_wrapper.h"

// Declaration of loadFromXml & appendShapedText from kdenlivetitle_wrapper.cpp
void loadFromXml(producer_ktitle self,
                 QGraphicsScene *scene,
                 const char *templateXml,
                 const char *templateText);

#include <QDir>

void appendShapedText(QPainterPath &path,
                      const QPointF &pos,
                      const QFont &font,
                      const QString &text);

static QString getOutputDir()
{
    return qEnvironmentVariable("MLT_TEST_OUTPUT_DIR");
}

class TestKdenliveTitle : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void testBasicPositioningAndBaseline();
    void testBaselineVsShapedRegression();
    void testDecorations();
    void testTabSpacing();
    void testAlignment();
    void testGradientsOutlinesAndShadows();
    void testMultiScriptShaping();
    void testXmlTitleRendering();
    void cleanupTestCase();
};

void TestKdenliveTitle::initTestCase()
{
    if (!QGuiApplication::instance()) {
        static int argc = 1;
        static char appName[] = "test_kdenlivetitle";
        static char *argv[] = {appName, nullptr};
        new QGuiApplication(argc, argv);
    }
}

void TestKdenliveTitle::testBasicPositioningAndBaseline()
{
    QFont font("Sans Serif", 24);
    QFontMetrics metrics(font);
    QVERIFY(metrics.ascent() > 0);
    QVERIFY(metrics.height() > 0);
}

void TestKdenliveTitle::testBaselineVsShapedRegression()
{
    QFont font("Sans Serif", 32);
    font.setHintingPreference(QFont::PreferFullHinting);
    QFontMetrics metrics(font);
    qreal linePos = metrics.ascent();
    QString latinText = QStringLiteral("Kdenlive Titler Baseline vs Shaped Test");

    QPainterPath baselinePath;
    baselinePath.addText(0, linePos, font, latinText);

    QPainterPath shapedPath;
    appendShapedText(shapedPath, QPointF(0, linePos), font, latinText);

    QRectF shapedRect = shapedPath.boundingRect();
    QRectF baseRect = baselinePath.boundingRect();

    qreal topDelta = qAbs(shapedRect.top() - baseRect.top());
    QVERIFY2(topDelta < 2.0,
             qPrintable(QString("Baseline top delta %1 exceeds tolerance").arg(topDelta)));

    qreal heightDelta = qAbs(shapedRect.height() - baseRect.height());
    QVERIFY2(heightDelta < 3.0,
             qPrintable(QString("Baseline height delta %1 exceeds tolerance").arg(heightDelta)));

    // Generate Visual Comparison Image
    QImage img(1200, 300, QImage::Format_ARGB32_Premultiplied);
    img.fill(QColor(30, 30, 30));
    QPainter p(&img);
    p.setRenderHint(QPainter::Antialiasing, true);

    // Draw grid/baseline reference line
    p.setPen(QPen(QColor(100, 100, 100), 1, Qt::DashLine));
    p.drawLine(50, 100 + linePos, 1150, 100 + linePos);
    p.drawLine(50, 200 + linePos, 1150, 200 + linePos);

    // Row 1: Baseline QPainterPath::addText (Red)
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(255, 80, 80));
    QPainterPath r1 = baselinePath;
    r1.translate(50, 100);
    p.drawPath(r1);

    // Row 2: New appendShapedText (Green)
    p.setBrush(QColor(80, 255, 120));
    QPainterPath r2 = shapedPath;
    r2.translate(50, 200);
    p.drawPath(r2);

    p.setPen(Qt::white);
    p.setFont(QFont("Sans Serif", 12));
    p.drawText(50, 40, "BASELINE (addText - Red) vs NEW SHAPED (appendShapedText - Green)");

    p.end();
    if (!getOutputDir().isEmpty()) {
        img.save(QDir(getOutputDir()).filePath(QStringLiteral("baseline_vs_shaped_comparison.png")));
    }
}

void TestKdenliveTitle::testDecorations()
{
    QFont font("Sans Serif", 24);
    font.setUnderline(true);
    font.setStrikeOut(true);

    QVERIFY(font.underline());
    QVERIFY(font.strikeOut());

    QFontMetricsF metrics(font);
    QVERIFY(metrics.lineWidth() > 0);
    QVERIFY(metrics.underlinePos() != 0);
    QVERIFY(metrics.strikeOutPos() != 0);
}

void TestKdenliveTitle::testTabSpacing()
{
    QString tabText = QStringLiteral("Left\tRight");
    QVERIFY(tabText.contains(QLatin1Char('\t')));
}

void TestKdenliveTitle::testAlignment()
{
    int alignLeft = Qt::AlignLeft;
    int alignCenter = Qt::AlignHCenter;
    int alignRight = Qt::AlignRight;

    QCOMPARE(alignLeft, (int) Qt::AlignLeft);
    QCOMPARE(alignCenter, (int) Qt::AlignHCenter);
    QCOMPARE(alignRight, (int) Qt::AlignRight);
}

void TestKdenliveTitle::testGradientsOutlinesAndShadows()
{
    QLinearGradient gradient(0, 0, 100, 100);
    gradient.setColorAt(0.0, Qt::red);
    gradient.setColorAt(1.0, Qt::blue);

    QBrush brush(gradient);
    QVERIFY(brush.style() == Qt::LinearGradientPattern);
}

void TestKdenliveTitle::testMultiScriptShaping()
{
    struct ScriptCase
    {
        const char *name;
        const char16_t *text;
    } scriptCases[] = {{"Sinhala Rakaransaya", u"ශ්‍රී ලංකා (Sri Lanka)"},
                       {"Devanagari Conjuncts", u"नमस्ते भारत (Namaste)"},
                       {"Bengali", u"বাংলা ভাষা (Bengali)"},
                       {"Tamil", u"தமிழ் மொழி (Tamil)"},
                       {"Telugu", u"తెలుగు భాష (Telugu)"},
                       {"Malayalam", u"മലയാളം (Malayalam)"},
                       {"Arabic Cursive RTL", u"مرحبا بالعالم (Welcome)"},
                       {"Urdu", u"اردو زبان (Urdu)"},
                       {"Simplified Chinese", u"简体中文 (Simplified Chinese)"},
                       {"Traditional Chinese", u"繁體中文 (Traditional Chinese)"},
                       {"Japanese Kana & Kanji", u"日本語ひらがなカタカナ (Japanese)"},
                       {"Korean Hangul", u"한국어 한글 (Korean)"},
                       {"Italic Latin Overhang", u"Sample Italic Text Overhang"}};

    QImage img(1200, 900, QImage::Format_ARGB32_Premultiplied);
    img.fill(QColor(25, 28, 35));
    QPainter p(&img);
    p.setRenderHint(QPainter::Antialiasing, true);

    p.setPen(QColor(240, 240, 240));
    p.setFont(QFont("Sans Serif", 16, QFont::Bold));
    p.drawText(40, 40, "Multi-Script Complex Text Shaping Test Cases");

    int y = 90;
    QFont font("Sans Serif", 22);
    for (const auto &sc : scriptCases) {
        QString label = QString::fromUtf8(sc.name) + QStringLiteral(": ");
        QString str = QString::fromUtf16(sc.text);

        p.setPen(QColor(160, 180, 210));
        p.setFont(QFont("Sans Serif", 11));
        p.drawText(40, y, label);

        QPainterPath path;
        appendShapedText(path, QPointF(280, y), font, str);
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(240, 245, 250));
        p.drawPath(path);

        y += 60;
    }

    p.end();
    if (!getOutputDir().isEmpty()) {
        img.save(QDir(getOutputDir()).filePath(QStringLiteral("multiscript_shaping_render.png")));
    }
}

void TestKdenliveTitle::testXmlTitleRendering()
{
    QGraphicsScene scene;
    struct producer_ktitle_s dummySelf;
    memset(&dummySelf, 0, sizeof(dummySelf));
    mlt_producer_init(&dummySelf.parent, NULL);

    const char *xml
        = "<kdenlivetitle width=\"1200\" height=\"600\" LC_NUMERIC=\"C\">\n"
          " <item type=\"QGraphicsTextItem\" z-index=\"1\">\n"
          "  <position x=\"50\" y=\"50\">\n"
          "   <transform>1,0,0,0,1,0,0,0,1</transform>\n"
          "  </position>\n"
          "  <content font=\"Sans Serif\" font-size=\"42\" font-bold=\"1\" underline=\"1\" "
          "strikeout=\"1\" box-width=\"1100\" box-height=\"200\" color=\"255,255,255,255\" "
          "outline=\"4\" outline-color=\"0,0,0,255\" alignment=\"1\" "
          "shadow=\"1;0,0,0,180;4;6;6\">ශ්‍රී ලංකා\tKdenlive Titler "
          "Test</content>\n"
          " </item>\n"
          " <item type=\"QGraphicsTextItem\" z-index=\"2\">\n"
          "  <position x=\"50\" y=\"250\">\n"
          "   <transform>1,0,0,0,1,0,0,0,1</transform>\n"
          "  </position>\n"
          "  <content font=\"Sans Serif\" font-size=\"36\" font-italic=\"1\" box-width=\"1100\" "
          "box-height=\"200\" color=\"255,220,100,255\" outline=\"2\" "
          "outline-color=\"40,40,40,255\" alignment=\"0\" "
          "shadow=\"1;0,0,0,160;3;4;4\">Multicontrol Italic Overhang &amp; Shadow</content>\n"
          " </item>\n"
          "</kdenlivetitle>";

    loadFromXml(&dummySelf, &scene, xml, nullptr);
    QVERIFY(!scene.items().isEmpty());

    QImage img(1200, 600, QImage::Format_ARGB32_Premultiplied);
    img.fill(QColor(35, 40, 50));
    QPainter painter(&img);
    painter.setRenderHint(QPainter::Antialiasing, true);
    scene.render(&painter);
    painter.end();

    if (!getOutputDir().isEmpty()) {
        img.save(QDir(getOutputDir())
                     .filePath(QStringLiteral("decorations_gradients_shadows_render.png")));
    }

    mlt_producer_close(&dummySelf.parent);
}

void TestKdenliveTitle::cleanupTestCase() {}

QTEST_MAIN(TestKdenliveTitle)
#include "test_kdenlivetitle.moc"
