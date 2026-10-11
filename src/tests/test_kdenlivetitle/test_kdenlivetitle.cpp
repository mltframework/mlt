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

#include <cmath>
#include <QDebug>
#include <QDir>
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
#include "../../src/modules/qt/typewriter.h"

// Declarations of functions exported / accessible from kdenlivetitle_wrapper.cpp
void loadFromXml(producer_ktitle self,
                 QGraphicsScene *scene,
                 const char *templateXml,
                 const char *templateText);

void appendShapedText(QPainterPath &path,
                      const QPointF &pos,
                      const QFont &font,
                      const QString &text);

static QString getOutputDir()
{
    return qEnvironmentVariable("MLT_TEST_OUTPUT_DIR");
}

// Helper: Render title XML via loadFromXml and return rendered QImage
static QImage renderTitleXml(const QString &xml, int width = 1280, int height = 360)
{
    struct producer_ktitle_s dummySelf;
    memset(&dummySelf, 0, sizeof(dummySelf));
    mlt_producer_init(&dummySelf.parent, nullptr);

    QGraphicsScene scene;
    loadFromXml(&dummySelf, &scene, xml.toUtf8().constData(), nullptr);

    QImage img(width, height, QImage::Format_ARGB32_Premultiplied);
    img.fill(Qt::transparent);
    QPainter p(&img);
    p.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);
    scene.render(&p,
                 QRectF(0, 0, width, height),
                 QRectF(0, 0, width, height),
                 Qt::IgnoreAspectRatio);
    p.end();

    mlt_producer_close(&dummySelf.parent);
    return img;
}

// Helper: Calculate ink bounding rect for pixels with alpha >= minAlpha
static QRect inkBounds(const QImage &img, int minAlpha = 32)
{
    int minX = img.width(), maxX = -1;
    int minY = img.height(), maxY = -1;
    for (int y = 0; y < img.height(); ++y) {
        for (int x = 0; x < img.width(); ++x) {
            if (qAlpha(img.pixel(x, y)) >= minAlpha) {
                minX = qMin(minX, x);
                maxX = qMax(maxX, x);
                minY = qMin(minY, y);
                maxY = qMax(maxY, y);
            }
        }
    }
    if (maxX < minX || maxY < minY) {
        return QRect();
    }
    return QRect(minX, minY, maxX - minX + 1, maxY - minY + 1);
}

// Helper: Count ink pixels with alpha >= minAlpha
static int countInkPixels(const QImage &img, int minAlpha = 32)
{
    int count = 0;
    for (int y = 0; y < img.height(); ++y) {
        for (int x = 0; x < img.width(); ++x) {
            if (qAlpha(img.pixel(x, y)) >= minAlpha) {
                count++;
            }
        }
    }
    return count;
}

// Helper: Build standard test title XML
static QString makeTitleXml(const QString &attrs,
                            const QString &text,
                            int width = 1280,
                            int height = 360,
                            int boxW = 1200,
                            int boxH = 280)
{
    return QStringLiteral(
               "<kdenlivetitle width=\"%1\" height=\"%2\" LC_NUMERIC=\"C\">\n"
               " <item type=\"QGraphicsTextItem\" z-index=\"1\">\n"
               "  <position x=\"40\" y=\"40\">\n"
               "   <transform>1,0,0,0,1,0,0,0,1</transform>\n"
               "  </position>\n"
               "  <content box-width=\"%3\" box-height=\"%4\" font-color=\"255,255,255,255\" "
               "font-weight=\"400\" %5>%6</content>\n"
               " </item>\n"
               "</kdenlivetitle>")
        .arg(width)
        .arg(height)
        .arg(boxW)
        .arg(boxH)
        .arg(attrs, text.toHtmlEscaped());
}

class TestKdenliveTitle : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void testBaselineAlignmentVsAddText();
    void testDecorationsUnderlineAndStrikeout();
    void testTabSpacingAndOffsets();
    void testHorizontalAlignmentLeftCenterRight();
    void testMultilineAndLineSpacing();
    void testFontStylesAndAccentedLatin();
    void testGradientsOutlinesAndDropShadows();
    void testTypewriterAnimationProgression();
    void testRichTextCompatibility();
    void testComplexScriptShapingMatrix();
    void testRealWorldSavedTitleXml();
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

void TestKdenliveTitle::testBaselineAlignmentVsAddText()
{
    // Verify that appendShapedText glyphs align vertically with QPainterPath::addText
    // across multiple font sizes without any baseline shift (dTop == 0).
    for (int pointSize : {16, 24, 32, 48, 64}) {
        QFont font("Sans Serif", pointSize);
        font.setHintingPreference(QFont::PreferFullHinting);
        QFontMetricsF metrics(font);
        qreal linePos = metrics.ascent();
        QString latinText = QStringLiteral("Kdenlive Titler Baseline Test 123");

        QPainterPath basePath;
        basePath.addText(0, linePos, font, latinText);

        QPainterPath shapedPath;
        appendShapedText(shapedPath, QPointF(0, linePos), font, latinText);

        QRectF baseRect = basePath.boundingRect();
        QRectF shapedRect = shapedPath.boundingRect();

        // Baseline vertical bounds must match within sub-pixel anti-aliasing tolerance
        qreal topDelta = qAbs(shapedRect.top() - baseRect.top());
        qreal heightDelta = qAbs(shapedRect.height() - baseRect.height());

        QVERIFY2(topDelta < 1.5,
                 qPrintable(QString("At size %1, topDelta %2 exceeds tolerance")
                                .arg(pointSize)
                                .arg(topDelta)));
        QVERIFY2(heightDelta < 2.0,
                 qPrintable(QString("At size %1, heightDelta %2 exceeds tolerance")
                                .arg(pointSize)
                                .arg(heightDelta)));
    }
}

void TestKdenliveTitle::testDecorationsUnderlineAndStrikeout()
{
    // 1. Path-level geometry verification
    QFont plainFont("Sans Serif", 32);
    QFont underlineFont = plainFont;
    underlineFont.setUnderline(true);
    QFont strikeFont = plainFont;
    strikeFont.setStrikeOut(true);

    QString text = QStringLiteral("DecoratedText");
    qreal baseline = 60.0;
    QPainterPath plainPath, underlinePath, strikePath;
    appendShapedText(plainPath, QPointF(0, baseline), plainFont, text);
    appendShapedText(underlinePath, QPointF(0, baseline), underlineFont, text);
    appendShapedText(strikePath, QPointF(0, baseline), strikeFont, text);

    QFontMetricsF metrics(plainFont);
    qreal expectedUnderlineY = baseline + metrics.underlinePos();
    qreal expectedStrikeY = baseline - metrics.strikeOutPos();

    QVERIFY(underlinePath.boundingRect().bottom() >= expectedUnderlineY);
    QVERIFY(strikePath.boundingRect().top() <= expectedStrikeY);

    // 2. Rendered pixel-level verification via title XML
    QString plainXml = makeTitleXml(QStringLiteral("font=\"Sans Serif\" font-pixel-size=\"48\""),
                                    text);
    QString underXml
        = makeTitleXml(QStringLiteral(
                           "font=\"Sans Serif\" font-pixel-size=\"48\" font-underline=\"1\""),
                       text);

    QImage imgPlain = renderTitleXml(plainXml);
    QImage imgUnder = renderTitleXml(underXml);

    QRect boundsPlain = inkBounds(imgPlain);
    QRect boundsUnder = inkBounds(imgUnder);

    // Underline extends ink below plain text
    QVERIFY(boundsUnder.bottom() > boundsPlain.bottom());

    // Inspect the underline row below plain text: must contain solid ink
    int underRow = boundsUnder.bottom();
    int underRowPixels = 0;
    for (int x = boundsUnder.left(); x <= boundsUnder.right(); ++x) {
        if (qAlpha(imgUnder.pixel(x, underRow)) > 128) {
            underRowPixels++;
        }
    }
    QVERIFY2(underRowPixels > 50,
             "Underline did not render solid ink pixels in the decoration line");
}

void TestKdenliveTitle::testTabSpacingAndOffsets()
{
    // Verify that tabs produce distinct column offsets and empty space in between
    QString textNoTab = QStringLiteral("FirstSecond");
    QString textTab = QStringLiteral("First\tSecond");

    QString xmlNoTab = makeTitleXml(QStringLiteral("font=\"Sans Serif\" font-pixel-size=\"36\""),
                                    textNoTab);
    QString xmlTab
        = makeTitleXml(QStringLiteral(
                           "font=\"Sans Serif\" font-pixel-size=\"36\" tab-width=\"200\""),
                       textTab);

    QImage imgNoTab = renderTitleXml(xmlNoTab);
    QImage imgTab = renderTitleXml(xmlTab);

    QRect boundsNoTab = inkBounds(imgNoTab);
    QRect boundsTab = inkBounds(imgTab);

    // Tabbed text must be strictly wider due to the tab advance stop
    QVERIFY(boundsTab.width() > boundsNoTab.width() + 50);

    // Verify that the tab stop aligns "Second" at or past position 40 + 200 = 240
    // "First" width in 36px font is ~80px. Tab stop at 200 pushes "Second" to x >= 240.
    QFont font("Sans Serif");
    font.setPixelSize(36);
    QFontMetrics fm(font);
    int firstAdvance = fm.horizontalAdvance("First");
    int itemOriginX = 40;
    int expectedSecondStart = itemOriginX + int(ceil(double(firstAdvance) / 200.0)) * 200;

    // Check that inside the tab gap (itemOriginX + firstAdvance + 5 to expectedSecondStart - 5),
    // pixels are transparent
    bool gapIsEmpty = true;
    for (int x = itemOriginX + firstAdvance + 5; x < expectedSecondStart - 5; ++x) {
        for (int y = boundsTab.top(); y <= boundsTab.bottom(); ++y) {
            if (qAlpha(imgTab.pixel(x, y)) > 64) {
                gapIsEmpty = false;
                break;
            }
        }
    }
    QVERIFY2(gapIsEmpty, "Tab gap between columns contains unexpected ink");
}

void TestKdenliveTitle::testHorizontalAlignmentLeftCenterRight()
{
    // Test container box-width=800 inside 1280 canvas at x=40
    QString text = QStringLiteral("Alignment Verification");
    QString leftXml
        = makeTitleXml(QStringLiteral("font=\"Sans Serif\" font-pixel-size=\"36\" alignment=\"1\""),
                       text,
                       1280,
                       360,
                       800,
                       280);
    QString centerXml
        = makeTitleXml(QStringLiteral("font=\"Sans Serif\" font-pixel-size=\"36\" alignment=\"4\""),
                       text,
                       1280,
                       360,
                       800,
                       280);
    QString rightXml
        = makeTitleXml(QStringLiteral("font=\"Sans Serif\" font-pixel-size=\"36\" alignment=\"2\""),
                       text,
                       1280,
                       360,
                       800,
                       280);

    QImage imgLeft = renderTitleXml(leftXml);
    QImage imgCenter = renderTitleXml(centerXml);
    QImage imgRight = renderTitleXml(rightXml);

    QRect boundsLeft = inkBounds(imgLeft);
    QRect boundsCenter = inkBounds(imgCenter);
    QRect boundsRight = inkBounds(imgRight);

    // 1. Left alignment: left bound near container origin x=40
    QVERIFY2(boundsLeft.left() >= 40 && boundsLeft.left() <= 45,
             qPrintable(QString("Left align ink start %1 not near 40").arg(boundsLeft.left())));

    // 2. Center alignment: horizontal center of ink should be near 40 + 800/2 = 440
    int centerMid = (boundsCenter.left() + boundsCenter.right()) / 2;
    QVERIFY2(qAbs(centerMid - 440) <= 2,
             qPrintable(QString("Center align midpoint %1 not near 440").arg(centerMid)));

    // 3. Right alignment: right bound near 40 + 800 = 840
    QVERIFY2(boundsRight.right() >= 835 && boundsRight.right() <= 845,
             qPrintable(QString("Right align ink end %1 not near 840").arg(boundsRight.right())));

    // 4. Strict monotonic ordering: Left < Center < Right
    QVERIFY(boundsLeft.left() < boundsCenter.left());
    QVERIFY(boundsCenter.left() < boundsRight.left());
}

void TestKdenliveTitle::testMultilineAndLineSpacing()
{
    QString text = QStringLiteral("First Line\nSecond Line");
    QString baseXml
        = makeTitleXml(QStringLiteral(
                           "font=\"Sans Serif\" font-pixel-size=\"36\" line-spacing=\"0\""),
                       text);
    QString spacedXml
        = makeTitleXml(QStringLiteral(
                           "font=\"Sans Serif\" font-pixel-size=\"36\" line-spacing=\"30\""),
                       text);

    QImage imgBase = renderTitleXml(baseXml);
    QImage imgSpaced = renderTitleXml(spacedXml);

    QRect boundsBase = inkBounds(imgBase);
    QRect boundsSpaced = inkBounds(imgSpaced);

    // Extra line spacing must increase overall height by exactly 30 pixels (+/- 1px)
    int heightDelta = boundsSpaced.height() - boundsBase.height();
    QVERIFY2(qAbs(heightDelta - 30) <= 1,
             qPrintable(
                 QString("Line spacing height delta %1 differs from expected 30").arg(heightDelta)));
}

void TestKdenliveTitle::testFontStylesAndAccentedLatin()
{
    QString text = QStringLiteral("Héllò Wörld ñ ç ÁÉÍÓÚ");
    QString regularXml = makeTitleXml(QStringLiteral("font=\"Sans Serif\" font-pixel-size=\"36\""),
                                      text);
    QString boldXml
        = makeTitleXml(QStringLiteral("font=\"Sans Serif\" font-pixel-size=\"36\" font-bold=\"1\""),
                       text);
    QString italicXml
        = makeTitleXml(QStringLiteral(
                           "font=\"Sans Serif\" font-pixel-size=\"36\" font-italic=\"1\""),
                       text);

    QImage imgRegular = renderTitleXml(regularXml);
    QImage imgBold = renderTitleXml(boldXml);
    QImage imgItalic = renderTitleXml(italicXml);

    int pixelsReg = countInkPixels(imgRegular);
    int pixelsBold = countInkPixels(imgBold);
    QRect boundsReg = inkBounds(imgRegular);
    QRect boundsItalic = inkBounds(imgItalic);

    // Bold text must have significantly more ink pixels than regular text (> 15% increase)
    QVERIFY2(pixelsBold > int(pixelsReg * 1.15),
             qPrintable(QString("Bold pixels %1 did not exceed regular %2 significantly")
                            .arg(pixelsBold)
                            .arg(pixelsReg)));

    // Italic text slant extends ink bounds beyond regular right bound
    QVERIFY(boundsItalic.right() >= boundsReg.right());

    // Accented characters: verify diacritics rendered above capital height
    QFont font("Sans Serif");
    font.setPixelSize(36);
    QFontMetrics fm(font);
    int originY = 40;
    int baselineY = originY + fm.ascent();
    int capTopY = baselineY - fm.capHeight();

    // Accents on 'Á', 'É', etc. must produce ink above capTopY
    bool foundDiacritics = false;
    for (int y = boundsReg.top(); y < capTopY; ++y) {
        for (int x = boundsReg.left(); x <= boundsReg.right(); ++x) {
            if (qAlpha(imgRegular.pixel(x, y)) > 64) {
                foundDiacritics = true;
                break;
            }
        }
        if (foundDiacritics)
            break;
    }
    QVERIFY2(foundDiacritics, "Accented Latin diacritics did not render above capital height");
}

void TestKdenliveTitle::testGradientsOutlinesAndDropShadows()
{
    QString text = QStringLiteral("EFFECTS");
    // 1. Horizontal Linear Gradient: Red at left (0%) to Blue at right (100%)
    QString gradXml = makeTitleXml(
        QStringLiteral(
            "font=\"Sans Serif\" font-pixel-size=\"48\" gradient=\"#ff0000;#0000ff;0;100;0\""),
        text,
        1280,
        360,
        190,
        100);
    QImage imgGrad = renderTitleXml(gradXml);
    QRect bGrad = inkBounds(imgGrad);
    QVERIFY(!bGrad.isEmpty());

    // Left area should have red dominant, right area blue dominant
    QColor leftCol(imgGrad.pixel(bGrad.left() + 10, bGrad.center().y()));
    QColor rightCol(imgGrad.pixel(bGrad.right() - 10, bGrad.center().y()));
    QVERIFY(leftCol.red() > leftCol.blue());
    QVERIFY(rightCol.blue() > rightCol.red());

    // 2. Outline: White fill with thick red outline
    QString outXml = makeTitleXml(QStringLiteral(
                                      "font=\"Sans Serif\" font-pixel-size=\"48\" "
                                      "font-outline=\"8\" font-outline-color=\"255,0,0,255\""),
                                  text);
    QImage imgOut = renderTitleXml(outXml);
    QRect bOut = inkBounds(imgOut);

    QString plainXml = makeTitleXml(QStringLiteral("font=\"Sans Serif\" font-pixel-size=\"48\""),
                                    text);
    QImage imgPlain = renderTitleXml(plainXml);
    QRect bPlain = inkBounds(imgPlain);

    // 8px outline must expand ink bounds in all directions by ~4px
    QVERIFY(bOut.left() < bPlain.left());
    QVERIFY(bOut.right() > bPlain.right());
    QVERIFY(bOut.top() < bPlain.top());
    QVERIFY(bOut.bottom() > bPlain.bottom());

    // 3. Drop Shadow: Offset shadow (+8, +8)
    QString shadowXml = makeTitleXml(
        QStringLiteral("font=\"Sans Serif\" font-pixel-size=\"48\" shadow=\"1;#c0000000;0;8;8\""),
        text);
    QImage imgShadow = renderTitleXml(shadowXml);
    QRect bShadow = inkBounds(imgShadow);

    // Shadow must extend bounds right and downward by ~8px
    QVERIFY(bShadow.right() >= bPlain.right() + 6);
    QVERIFY(bShadow.bottom() >= bPlain.bottom() + 6);
}

void TestKdenliveTitle::testTypewriterAnimationProgression()
{
    // Test typewriter effect progression across frames using character macro :c{text}
    std::string phrase = "Typewriter Animation";
    TypeWriter tw;
    tw.setPattern(":c{" + phrase + "}");
    tw.setFrameStep(2);
    tw.parse();

    std::string f0 = tw.render(0);
    std::string f10 = tw.render(10);
    std::string f20 = tw.render(20);
    std::string f40 = tw.render(40);

    // Strings must monotonically grow
    QVERIFY(f0.length() <= f10.length());
    QVERIFY(f10.length() <= f20.length());
    QVERIFY(f20.length() <= f40.length());
    QCOMPARE(QString::fromStdString(f40), QString::fromStdString(phrase));

    // Render into titles and verify rendered width grows monotonically
    QImage img0 = renderTitleXml(
        makeTitleXml(QStringLiteral("font=\"Sans Serif\" font-pixel-size=\"36\""),
                     QString::fromStdString(f0)));
    QImage img10 = renderTitleXml(
        makeTitleXml(QStringLiteral("font=\"Sans Serif\" font-pixel-size=\"36\""),
                     QString::fromStdString(f10)));
    QImage img40 = renderTitleXml(
        makeTitleXml(QStringLiteral("font=\"Sans Serif\" font-pixel-size=\"36\""),
                     QString::fromStdString(f40)));

    QRect b0 = inkBounds(img0);
    QRect b10 = inkBounds(img10);
    QRect b40 = inkBounds(img40);

    QVERIFY(b0.width() <= b10.width());
    QVERIFY(b10.width() < b40.width());
}

void TestKdenliveTitle::testRichTextCompatibility()
{
    // Test title XML containing rich text markup with mixed formatting
    const char *richXml
        = "<kdenlivetitle width=\"1280\" height=\"360\" LC_NUMERIC=\"C\">\n"
          " <item type=\"QGraphicsTextItem\" z-index=\"1\">\n"
          "  <position x=\"40\" y=\"40\"><transform>1,0,0,0,1,0,0,0,1</transform></position>\n"
          "  <content box-width=\"1200\" box-height=\"280\" font=\"Sans Serif\" "
          "font-pixel-size=\"36\">\n"
          "   <richtext format=\"qt-html-v1\">&lt;p&gt;&lt;span style=\"font-size:48px; "
          "color:#ff0000;\"&gt;Large Red&lt;/span&gt; &lt;span style=\"font-size:24px; "
          "color:#00ff00;\"&gt;Small Green&lt;/span&gt;&lt;/p&gt;</richtext>\n"
          "  </content>\n"
          " </item>\n"
          "</kdenlivetitle>";

    QImage img = renderTitleXml(QString::fromUtf8(richXml));
    QRect bounds = inkBounds(img);
    QVERIFY(!bounds.isEmpty());

    // Verify both red and green colors are present in the rendered image
    bool foundRed = false;
    bool foundGreen = false;
    for (int y = bounds.top(); y <= bounds.bottom(); ++y) {
        for (int x = bounds.left(); x <= bounds.right(); ++x) {
            QColor c(img.pixel(x, y));
            if (c.alpha() > 180) {
                if (c.red() > 200 && c.green() < 50 && c.blue() < 50)
                    foundRed = true;
                if (c.green() > 200 && c.red() < 50 && c.blue() < 50)
                    foundGreen = true;
            }
        }
    }
    QVERIFY2(foundRed, "Rich text red span was not rendered");
    QVERIFY2(foundGreen, "Rich text green span was not rendered");
}

void TestKdenliveTitle::testComplexScriptShapingMatrix()
{
    struct ScriptCase
    {
        const char *scriptName;
        const char16_t *text;
    } cases[] = {{"Sinhala (Rakaransaya)", u"ශ්‍රී ලංකා"},
                 {"Devanagari (Conjuncts)", u"नमस्ते भारत"},
                 {"Bengali", u"বাংলা ভাষা"},
                 {"Tamil", u"தமிழ் மொழி"},
                 {"Telugu", u"తెలుగు భాష"},
                 {"Malayalam", u"മലയാളം"},
                 {"Arabic (RTL Cursive)", u"مرحبا بالعالم"},
                 {"Urdu", u"اردو زبان"},
                 {"Simplified Chinese", u"简体中文测试"},
                 {"Traditional Chinese", u"繁體中文測試"},
                 {"Japanese", u"日本語テスト"},
                 {"Korean (Hangul)", u"한국어 테스트"}};

    QFont font("Sans Serif", 36);

    for (const auto &sc : cases) {
        QString text = QString::fromUtf16(sc.text);
        QPainterPath path;
        appendShapedText(path, QPointF(0, 50), font, text);

        // Path must not be empty
        QVERIFY2(!path.isEmpty(),
                 qPrintable(QString("%1: Shaped path was empty").arg(sc.scriptName)));

        // Path bounds must be positive
        QRectF bounds = path.boundingRect();
        QVERIFY2(bounds.width() > 30.0,
                 qPrintable(QString("%1: Shaped width %2 is unreasonably small")
                                .arg(sc.scriptName)
                                .arg(bounds.width())));
        QVERIFY2(bounds.height() > 15.0,
                 qPrintable(QString("%1: Shaped height %2 is unreasonably small")
                                .arg(sc.scriptName)
                                .arg(bounds.height())));

        // Render via title XML and assert non-empty raster
        QString xml = makeTitleXml(QStringLiteral("font=\"Sans Serif\" font-pixel-size=\"36\""),
                                   text);
        QImage img = renderTitleXml(xml);
        QRect rBounds = inkBounds(img);
        QVERIFY2(!rBounds.isEmpty(),
                 qPrintable(QString("%1: Rendered title image has no ink").arg(sc.scriptName)));
    }
}

void TestKdenliveTitle::testRealWorldSavedTitleXml()
{
    // Realistic multi-element saved title clip: complex script + Latin subtitle + outline + shadow
    const char *xml
        = "<kdenlivetitle width=\"1920\" height=\"1080\" LC_NUMERIC=\"C\">\n"
          " <item type=\"QGraphicsRectItem\" z-index=\"0\">\n"
          "  <position x=\"100\" y=\"800\"><transform>1,0,0,0,1,0,0,0,1</transform></position>\n"
          "  <content brushcolor=\"0,0,0,180\" penwidth=\"0\" penstyle=\"0\" rect=\"0,0,1720,180\" "
          "/>\n"
          " </item>\n"
          " <item type=\"QGraphicsTextItem\" z-index=\"1\">\n"
          "  <position x=\"140\" y=\"820\"><transform>1,0,0,0,1,0,0,0,1</transform></position>\n"
          "  <content box-width=\"1640\" box-height=\"70\" font=\"Sans Serif\" "
          "font-pixel-size=\"42\" font-bold=\"1\" "
          "font-color=\"255,255,255,255\" font-outline=\"4\" font-outline-color=\"0,0,0,255\" "
          "alignment=\"1\" "
          "shadow=\"1;0,0,0,200;4;6;6\">ශ්‍රී ලංකා ජාතික රූපවාහිනිය (National "
          "Television)</content>\n"
          " </item>\n"
          " <item type=\"QGraphicsTextItem\" z-index=\"2\">\n"
          "  <position x=\"140\" y=\"900\"><transform>1,0,0,0,1,0,0,0,1</transform></position>\n"
          "  <content box-width=\"1640\" box-height=\"50\" font=\"Sans Serif\" "
          "font-pixel-size=\"28\" font-italic=\"1\" "
          "font-color=\"240,240,160,255\" alignment=\"1\">Live Broadcast • News &amp; Current "
          "Affairs</content>\n"
          " </item>\n"
          "</kdenlivetitle>";

    QImage img = renderTitleXml(QString::fromUtf8(xml), 1920, 1080);
    QRect bounds = inkBounds(img);

    // Title canvas must render items at the lower-third coordinates (around y=800..980)
    QVERIFY(!bounds.isEmpty());
    QVERIFY(bounds.top() >= 790 && bounds.top() <= 810);
    QVERIFY(bounds.bottom() >= 940 && bounds.bottom() <= 1000);
    QVERIFY(bounds.left() >= 95 && bounds.left() <= 105);
    QVERIFY(bounds.right() >= 1810 && bounds.right() <= 1830);

    if (!getOutputDir().isEmpty()) {
        img.save(QDir(getOutputDir()).filePath(QStringLiteral("realworld_title_render.png")));
    }
}

void TestKdenliveTitle::cleanupTestCase() {}

QTEST_MAIN(TestKdenliveTitle)
#include "test_kdenlivetitle.moc"
