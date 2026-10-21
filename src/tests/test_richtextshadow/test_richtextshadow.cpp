// SPDX-FileCopyrightText: 2026 klg90 <284782940+klg90@users.noreply.github.com>
// SPDX-License-Identifier: LGPL-2.1-or-later

#include <QAbstractTextDocumentLayout>
#include <QDir>
#include <QDomDocument>
#include <QFile>
#include <QFont>
#include <QImage>
#include <QPainter>
#include <QTextBlockFormat>
#include <QTextCursor>
#include <QTextDocument>
#include <QtTest>

#include <memory>
#include <mlt++/Mlt.h>
#include <modules/qt/richtextshadow.h>
#include <modules/qt/richtextspacing.h>

namespace {

void configureDocument(QTextDocument &document, int start, Qt::Alignment alignment = Qt::AlignLeft)
{
    QFont font(QStringLiteral("DejaVu Sans"));
    font.setPixelSize(64);
    document.setDefaultFont(font);
    document.setDocumentMargin(0);
    document.setPlainText(QStringLiteral("MMMM XXXX"));
    document.setTextWidth(600);
    QTextCursor cursor(&document);
    cursor.select(QTextCursor::Document);
    QTextCharFormat white;
    white.setForeground(QBrush(Qt::white));
    cursor.mergeCharFormat(white);
    QTextBlockFormat block;
    block.setAlignment(alignment);
    cursor.mergeBlockFormat(block);
    cursor.setPosition(start);
    cursor.setPosition(start + 4, QTextCursor::KeepAnchor);
    QTextCharFormat gradient;
    gradient.setProperty(TitlerShadowV1::Enabled, true);
    gradient.setProperty(TitlerShadowV1::Color, QColor(0, 255, 0));
    gradient.setProperty(TitlerShadowV1::Blur, 0);
    gradient.setProperty(TitlerShadowV1::OffsetX, 0);
    gradient.setProperty(TitlerShadowV1::OffsetY, 100);
    cursor.mergeCharFormat(gradient);
}

QImage renderDocument(QTextDocument &document)
{
    QImage image(1280, 720, QImage::Format_RGBA8888);
    image.fill(Qt::transparent);
    QPainter painter(&image);
    painter.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);
    painter.translate(40, 40);
    TitlerShadowV1::paint(&painter, TitlerShadowV1::render(&document, {}, Qt::NoPen));
    TitlerOutline::paint(&painter, &document, Qt::NoPen);
    // QGraphicsTextItem exposes ink outside the document rectangle (accents
    // and their overline). Use that same source clip for this Qt reference.
    const qreal margin = TitlerOutline::margin(&document, Qt::NoPen);
    const QRectF bodyBounds = QRectF(QPointF(), document.size())
                                  .united(TitlerOutline::inkBounds(&document))
                                  .adjusted(-margin, -margin, margin, margin);
    painter.setClipRect(bodyBounds, Qt::IntersectClip);
    QAbstractTextDocumentLayout::PaintContext context;
    context.clip = bodyBounds;
    document.documentLayout()->draw(&painter, context);
    painter.end();
    return image;
}

QImage coloredPixels(const QImage &image)
{
    QImage selected(image.size(), QImage::Format_RGBA8888);
    selected.fill(Qt::transparent);
    for (int y = 0; y < image.height(); ++y) {
        for (int x = 0; x < image.width(); ++x) {
            const QColor color = image.pixelColor(x, y);
            if (color.alpha() > 0 && color.green() > color.red() + 60
                && color.green() > color.blue() + 60)
                selected.setPixelColor(x, y, color);
        }
    }
    return selected;
}

QDomDocument titleXml(QTextDocument &document, Qt::Alignment alignment, bool animate = false)
{
    QDomDocument xml;
    auto root = xml.createElement(QStringLiteral("kdenlivetitle"));
    root.setAttribute(QStringLiteral("width"), 1280);
    root.setAttribute(QStringLiteral("height"), 720);
    root.setAttribute(QStringLiteral("out"), 299);
    xml.appendChild(root);
    auto item = xml.createElement(QStringLiteral("item"));
    item.setAttribute(QStringLiteral("type"), QStringLiteral("QGraphicsTextItem"));
    item.setAttribute(QStringLiteral("z-index"), 0);
    root.appendChild(item);
    auto position = xml.createElement(QStringLiteral("position"));
    position.setAttribute(QStringLiteral("x"), 40);
    position.setAttribute(QStringLiteral("y"), 40);
    auto transform = xml.createElement(QStringLiteral("transform"));
    transform.appendChild(xml.createTextNode(QStringLiteral("1,0,0,0,1,0,0,0,1")));
    position.appendChild(transform);
    item.appendChild(position);
    auto content = xml.createElement(QStringLiteral("content"));
    content.setAttribute(QStringLiteral("font"), QStringLiteral("DejaVu Sans"));
    content.setAttribute(QStringLiteral("font-pixel-size"), 64);
    content.setAttribute(QStringLiteral("font-weight"), int(QFont::Normal));
    content.setAttribute(QStringLiteral("font-color"), QStringLiteral("255,255,255,255"));
    content.setAttribute(QStringLiteral("font-outline"), 0);
    content.setAttribute(QStringLiteral("font-outline-color"), QStringLiteral("0,0,0,255"));
    content.setAttribute(QStringLiteral("box-width"), 600);
    content.setAttribute(QStringLiteral("box-height"), document.size().height());
    content.setAttribute(QStringLiteral("preferred-width"), 600);
    content.setAttribute(QStringLiteral("alignment"), int(alignment));
    content.setAttribute(QStringLiteral("typewriter"),
                         animate ? QStringLiteral("1;2;1;0;37") : QStringLiteral("0;2;1;0;37"));
    content.appendChild(xml.createTextNode(document.toPlainText()));
    auto rich = xml.createElement(QStringLiteral("richtext"));
    rich.setAttribute(QStringLiteral("format"), QStringLiteral("qt-html-v1"));
    rich.appendChild(xml.createTextNode(document.toHtml()));
    content.appendChild(rich);
    content.appendChild(TitlerGradientV1::save(xml, &document));
    content.appendChild(TitlerOutline::save(xml, &document));
    content.appendChild(TitlerShadowV1::save(xml, &document));
    content.appendChild(TitlerSpacingV1::save(xml, &document));
    item.appendChild(content);
    auto background = xml.createElement(QStringLiteral("background"));
    background.setAttribute(QStringLiteral("color"), QStringLiteral("0,0,0,0"));
    root.appendChild(background);
    return xml;
}

void setTitle(Mlt::Producer &producer, const QDomDocument &xml)
{
    producer.set("xmldata", xml.toByteArray().constData());
    producer.set("length", 300);
    producer.set_in_and_out(0, 299);
    producer.set("force_reload", 1);
}

void saveFailureImages(const QImage &expected, const QImage &actual, const QDomDocument &xml)
{
    if (expected == actual)
        return;
    const QString output = qEnvironmentVariable("SHADOW_TEST_ARTIFACTS");
    if (output.isEmpty())
        return;
    const QDir directory(output);
    if (!QDir().mkpath(output)) {
        qWarning() << "Cannot create shadow test artifact directory" << output;
        return;
    }
    const QString name = QString::fromLatin1(QTest::currentTestFunction()) + QLatin1Char('-')
                         + QString::fromLatin1(QTest::currentDataTag());
    expected.save(directory.filePath(name + QStringLiteral("-expected.png")));
    actual.save(directory.filePath(name + QStringLiteral("-actual.png")));
    QFile source(directory.filePath(name + QStringLiteral(".xml")));
    if (source.open(QIODevice::WriteOnly))
        source.write(xml.toByteArray());
    qWarning() << "Shadow frame comparison artifacts:" << directory.filePath(name);
}

QImage renderFrame(Mlt::Producer &producer, int position)
{
    producer.seek(position);
    std::unique_ptr<Mlt::Frame> frame(producer.get_frame());
    if (!frame || !frame->is_valid())
        return {};
    mlt_image_format format = mlt_image_rgba;
    int width = 1280, height = 720;
    auto *data = frame->get_image(format, width, height);
    if (!data || format != mlt_image_rgba || width != 1280 || height != 720)
        return {};
    return QImage(data, width, height, width * 4, QImage::Format_RGBA8888).copy();
}
} // namespace

class TestRichTextShadow : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void initTestCase()
    {
        auto *repository = Mlt::Factory::init();
        QVERIFY(repository);
        std::unique_ptr<Mlt::Properties> metadata(
            repository->metadata(mlt_service_producer_type, "kdenlivetitle"));
        QVERIFY(metadata && metadata->get_double("version") >= 9);
    }

    void nativeAlphaAcrossRasterTargets()
    {
        QTextDocument document;
        configureDocument(document, 0);
        QTextCursor cursor(&document);
        cursor.select(QTextCursor::Document);
        QTextCharFormat format;
        format.setProperty(TitlerShadowV1::Enabled, false);
        format.setProperty(TitlerShadowV1::Color, QColor(0, 255, 0));
        format.setProperty(TitlerShadowV1::Blur, 0);
        format.setProperty(TitlerShadowV1::OffsetX, 0);
        format.setProperty(TitlerShadowV1::OffsetY, 100);
        cursor.mergeCharFormat(format);
        // This is the native QTextDocument result in RGBA, the video raster format.
        const QImage body = renderDocument(document);
        QImage tinted = body;
        {
            QPainter painter(&tinted);
            painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
            painter.fillRect(tinted.rect(), Qt::green);
        }
        // Only Enabled changes after the native reference, preserving format boundaries.
        format = QTextCharFormat();
        format.setProperty(TitlerShadowV1::Enabled, true);
        cursor.mergeCharFormat(format);
        const auto layers = TitlerShadowV1::render(&document, {}, Qt::NoPen);
        QVERIFY(!layers.isEmpty());
        for (const auto rasterFormat : {QImage::Format_RGBA8888,
                                        QImage::Format_RGBA8888_Premultiplied,
                                        QImage::Format_ARGB32_Premultiplied}) {
            QImage expected(body.size(), rasterFormat);
            expected.fill(Qt::transparent);
            {
                QPainter painter(&expected);
                painter.drawImage(0, 100, tinted);
            }
            QImage actual(body.size(), rasterFormat);
            actual.fill(Qt::transparent);
            {
                QPainter painter(&actual);
                painter.translate(40, 40);
                TitlerShadowV1::paint(&painter, layers);
            }
            int covered = 0;
            int mismatched = 0;
            for (int y = 0; y < actual.height(); ++y)
                for (int x = 0; x < actual.width(); ++x) {
                    const int expectedAlpha = qAlpha(expected.pixel(x, y));
                    covered += expectedAlpha > 0;
                    mismatched += qAlpha(actual.pixel(x, y)) != expectedAlpha;
                }
            QVERIFY(covered > 100);
            QCOMPARE(mismatched, 0);
        }
    }

    void selectiveRendering_data()
    {
        QTest::addColumn<int>("start");
        QTest::addColumn<int>("blur");
        QTest::addColumn<int>("alignment");
        for (int start : {0, 5})
            for (int blur : {0, 3, 12})
                for (auto alignment : {Qt::AlignLeft, Qt::AlignHCenter, Qt::AlignRight})
                    QTest::newRow(
                        qPrintable(QString("%1-%2-%3").arg(start).arg(blur).arg(int(alignment))))
                        << start << blur << int(alignment);
    }
    void selectiveRendering()
    {
        QFETCH(int, start);
        QFETCH(int, blur);
        QFETCH(int, alignment);
        QTextDocument document;
        configureDocument(document, start, Qt::Alignment(alignment));
        QTextCursor cursor(&document);
        cursor.setPosition(start);
        cursor.setPosition(start + 4, QTextCursor::KeepAnchor);
        QTextCharFormat format;
        format.setProperty(TitlerShadowV1::Blur, blur);
        cursor.mergeCharFormat(format);
        const auto expected = renderDocument(document);
        const auto mask = coloredPixels(expected);
        bool found = false;
        const qreal boundary = 40 + document.begin().layout()->lineAt(0).cursorToX(5);
        for (int y = 120; y < mask.height(); ++y)
            for (int x = 0; x < mask.width(); ++x) {
                if (!qAlpha(mask.pixel(x, y)))
                    continue;
                found = true;
                QVERIFY(start == 0 ? x <= boundary + 2 * blur + 3 : x >= boundary - 2 * blur - 3);
            }
        QVERIFY(found);
        Mlt::Profile profile("atsc_720p_25");
        Mlt::Producer producer(profile, "kdenlivetitle", "");
        QVERIFY(producer.is_valid());
        setTitle(producer, titleXml(document, Qt::Alignment(alignment)));
        QCOMPARE(renderFrame(producer, 240), expected);
    }
    void multipleStylesAndUnicode_data()
    {
        QTest::addColumn<QString>("text");
        QTest::addColumn<int>("decoration");
        QTest::newRow("multiple-sizes-italic-spacing-outline")
            << QString("Hello amazing world") << 3;
        QTest::newRow("Arabic-RTL") << QString::fromUtf8("ABC مرحبا بالعالم XYZ") << 0;
        QTest::newRow("Arabic-underline") << QString::fromUtf8("ABC مرحبا بالعالم XYZ") << 1;
        QTest::newRow("Arabic-strike-overline") << QString::fromUtf8("ABC مرحبا بالعالم XYZ") << 2;
        QTest::newRow("emoji-underline") << QString::fromUtf8("Ä office 🙂 ffi") << 1;
        QTest::newRow("emoji-strike-overline") << QString::fromUtf8("Ä office 🙂 ffi") << 2;
        QTest::newRow("combining-emoji-ligature") << QString::fromUtf8("Ä office 🙂 ffi") << 0;
        QTest::newRow("multiline") << QString("MMMM\nXXX yyyy") << 0;
        QTest::newRow("underline") << QString("MMMM XXXX") << 1;
        QTest::newRow("mixed-size-underlines") << QString("Hello amazing world") << 5;
        QTest::newRow("strike-overline") << QString("MMMM XXXX") << 2;
        QTest::newRow("transparent-gradient") << QString("MMMM XXXX") << 4;
    }
    void multipleStylesAndUnicode()
    {
        QFETCH(QString, text);
        QFETCH(int, decoration);
        QTextDocument document;
        configureDocument(document, 0);
        document.setPlainText(text);
        QTextCursor cursor(&document);
        cursor.select(QTextCursor::Document);
        QTextCharFormat format;
        format.setForeground(Qt::white);
        if (decoration == 1 || decoration == 5)
            format.setFontUnderline(true);
        if (decoration == 2) {
            format.setFontStrikeOut(true);
            format.setFontOverline(true);
        }
        cursor.mergeCharFormat(format);
        if (decoration == 3 || decoration == 5) {
            cursor.setPosition(6);
            cursor.setPosition(13, QTextCursor::KeepAnchor);
            QTextCharFormat mixed;
            mixed.setProperty(QTextFormat::FontPixelSize, 80);
            mixed.setFontWeight(QFont::Bold);
            mixed.setFontItalic(true);
            mixed.setFontLetterSpacingType(QFont::AbsoluteSpacing);
            mixed.setFontLetterSpacing(2.5);
            mixed.setProperty(TitlerOutline::WidthProperty, 6.0);
            mixed.setProperty(TitlerOutline::ColorProperty, QColor(Qt::black));
            cursor.mergeCharFormat(mixed);
        }
        if (decoration == 4) {
            cursor.select(QTextCursor::Document);
            QTextCharFormat gradient;
            gradient.setProperty(TitlerGradientV1::Property, "#00ffffff;#ffffffff;0;100;0");
            cursor.mergeCharFormat(gradient);
        }
        cursor.select(QTextCursor::Document);
        QTextCharFormat shadow;
        shadow.setProperty(TitlerShadowV1::Enabled, true);
        shadow.setProperty(TitlerShadowV1::Color, QColor(0, 255, 0));
        shadow.setProperty(TitlerShadowV1::OffsetY, 100);
        cursor.mergeCharFormat(shadow);
        // Adjacent independent styles exercise separate negative/positive offset masks.
        cursor.setPosition(0);
        cursor.setPosition(1, QTextCursor::KeepAnchor);
        shadow = QTextCharFormat();
        shadow.setProperty(TitlerShadowV1::Color, QColor(0, 0, 255));
        shadow.setProperty(TitlerShadowV1::OffsetX, -20);
        shadow.setProperty(TitlerShadowV1::Blur, 3);
        cursor.mergeCharFormat(shadow);
        document.documentLayout()->documentSize();
        TitlerGradientV1::applyBrushes(&document, 600, int(document.size().height()));
        const auto before = document.toHtml();
        const int undo = document.availableUndoSteps();
        const auto expected = renderDocument(document);
        Mlt::Profile profile("atsc_720p_25");
        Mlt::Producer producer(profile, "kdenlivetitle", "");
        const auto xml = titleXml(document, Qt::AlignLeft);
        setTitle(producer, xml);
        const auto exported = renderFrame(producer, 240);
        saveFailureImages(expected, exported, xml);
        QCOMPARE(exported, expected);
        QCOMPARE(document.toHtml(), before);
        QCOMPARE(document.availableUndoSteps(), undo);
    }
    void persistenceAndAtomicValidation()
    {
        QTextDocument document;
        configureDocument(document, 5);
        QDomDocument xml;
        auto content = xml.createElement("content");
        xml.appendChild(content);
        content.appendChild(TitlerShadowV1::save(xml, &document));
        QTextDocument restored;
        restored.setDocumentMargin(0);
        restored.setHtml(document.toHtml());
        restored.setTextWidth(600);
        QVERIFY(TitlerShadowV1::restore(content, &restored));
        QCOMPARE(renderDocument(restored), renderDocument(document));
        const auto before = restored.toHtml();
        const auto properties = TitlerShadowV1::save(xml, &restored);
        auto bad = content.firstChildElement("richtext-shadows");
        auto second = bad.firstChildElement("run").cloneNode().toElement();
        bad.appendChild(second);
        QVERIFY(!TitlerShadowV1::restore(content, &restored));
        QCOMPARE(restored.toHtml(), before);
        QCOMPARE(TitlerShadowV1::save(xml, &restored).firstChildElement("run").attribute("start"),
                 properties.firstChildElement("run").attribute("start"));
        QTextDocument plain;
        plain.setPlainText("legacy");
        QVERIFY(TitlerShadowV1::save(xml, &plain).isNull());
    }
    void legacyFallbackAndSelectedDisable()
    {
        QTextDocument document;
        configureDocument(document, 0);
        QTextCursor cursor(&document);
        cursor.select(QTextCursor::Document);
        QTextCharFormat disabled;
        disabled.setProperty(TitlerShadowV1::Enabled, false);
        cursor.mergeCharFormat(disabled);
        cursor.setPosition(5);
        cursor.setPosition(9, QTextCursor::KeepAnchor);
        QTextCharFormat enabled;
        enabled.setProperty(TitlerShadowV1::Enabled, true);
        cursor.mergeCharFormat(enabled);
        const TitlerShadowV1::Style fallback{true, QColor(0, 255, 0), 0, 0, 100};
        const auto layers = TitlerShadowV1::render(&document, fallback, Qt::NoPen);
        QVERIFY(!layers.isEmpty());
        QImage image(1280, 720, QImage::Format_RGBA8888);
        image.fill(Qt::transparent);
        {
            QPainter painter(&image);
            painter.translate(40, 40);
            TitlerShadowV1::paint(&painter, layers);
        }
        const qreal boundary = 40 + document.begin().layout()->lineAt(0).cursorToX(5);
        bool found = false;
        for (int y = 0; y < image.height(); ++y)
            for (int x = 0; x < image.width(); ++x)
                if (qAlpha(image.pixel(x, y))) {
                    found = true;
                    QVERIFY(x >= boundary - 1);
                }
        QVERIFY(found);
    }
    void typewriterSeeking()
    {
        Mlt::Profile profile("atsc_720p_25");
        for (int start : {0, 5}) {
            QTextDocument document;
            configureDocument(document, start, Qt::AlignRight);
            const auto expected = renderDocument(document);
            const auto xml = titleXml(document, Qt::AlignRight);
            for (int mode : {1, 2, 3}) {
                auto animatedXml = titleXml(document, Qt::AlignRight, true);
                animatedXml.documentElement()
                    .firstChildElement("item")
                    .firstChildElement("content")
                    .setAttribute("typewriter", QString("1;2;%1;0;37").arg(mode));
                Mlt::Producer native(profile, "kdenlivetitle", "");
                setTitle(native, animatedXml);
                Mlt::Producer external(profile, "kdenlivetitle", "");
                setTitle(external, xml);
                Mlt::Filter effect(profile, "typewriter");
                QVERIFY(effect.is_valid());
                effect.set("macro_type", mode);
                effect.set("step_length", 2);
                effect.set("step_sigma", 0);
                effect.set("random_seed", 37);
                QCOMPARE(external.attach(effect), 0);
                for (auto *producer : {&native, &external}) {
                    const QByteArray before(producer->get("xmldata"));
                    const auto first = renderFrame(*producer, 0);
                    QVERIFY(!first.isNull());
                    if (start == 5 && mode != 3) {
                        const auto mask = coloredPixels(first);
                        bool found = false;
                        for (int y = 0; y < mask.height(); ++y)
                            for (int x = 0; x < mask.width(); ++x)
                                found |= qAlpha(mask.pixel(x, y)) > 0;
                        QVERIFY(!found);
                    }
                    QCOMPARE(renderFrame(*producer, 240), expected);
                    QCOMPARE(renderFrame(*producer, 0), first);
                    QCOMPARE(QByteArray(producer->get("xmldata")), before);
                }
            }
        }
    }
};
QTEST_MAIN(TestRichTextShadow)
#include "test_richtextshadow.moc"
