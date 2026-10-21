// SPDX-FileCopyrightText: 2026 klg90 <284782940+klg90@users.noreply.github.com>
// SPDX-License-Identifier: LGPL-2.1-or-later

#include <QAbstractTextDocumentLayout>
#include <QDomDocument>
#include <QFont>
#include <QImage>
#include <QPainter>
#include <QTextBlockFormat>
#include <QTextCursor>
#include <QTextDocument>
#include <QtTest>

#include <memory>
#include <mlt++/Mlt.h>
#include <modules/qt/richtextgradient.h>

namespace {
const QString recipe = QStringLiteral("#ffffff00;#ffff0000;0;100;0");

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
    gradient.setProperty(TitlerGradientV1::Property, recipe);
    cursor.mergeCharFormat(gradient);
}

QImage renderDocument(QTextDocument &document)
{
    QImage image(1280, 720, QImage::Format_RGBA8888);
    image.fill(Qt::transparent);
    QPainter painter(&image);
    painter.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);
    painter.translate(40, 40);
    document.documentLayout()->draw(&painter, {});
    painter.end();
    return image;
}

bool coversGradient(const QImage &image)
{
    int minimum = 255, maximum = 0, interiors = 0;
    for (int y = 0; y < image.height(); ++y) {
        for (int x = 0; x < image.width(); ++x) {
            const QColor color = image.pixelColor(x, y);
            if (color.alpha() > 240 && color.red() > 220 && color.blue() < 20) {
                minimum = qMin(minimum, color.green());
                maximum = qMax(maximum, color.green());
                ++interiors;
            }
        }
    }
    return interiors > 100 && minimum < 65 && maximum > 190;
}

QImage coloredPixels(const QImage &image)
{
    QImage selected(image.size(), QImage::Format_RGBA8888);
    selected.fill(Qt::transparent);
    for (int y = 0; y < image.height(); ++y) {
        for (int x = 0; x < image.width(); ++x) {
            const QColor color = image.pixelColor(x, y);
            if (color.alpha() > 0 && color.red() > 200 && color.blue() < 32)
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

class TestRichTextGradient : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase()
    {
        auto *repository = Mlt::Factory::init();
        QVERIFY(repository);
        std::unique_ptr<Mlt::Properties> metadata(
            repository->metadata(mlt_service_producer_type, "kdenlivetitle"));
        QVERIFY2(metadata && metadata->get_double("version") >= 9,
                 "The tested Qt module must provide the rich-text kdenlivetitle producer (version "
                 ">= 9)");
    }

    void selectiveExport_data()
    {
        QTest::addColumn<int>("start");
        QTest::addColumn<int>("alignment");
        for (int start : {0, 5}) {
            for (int alignment : {int(Qt::AlignLeft), int(Qt::AlignHCenter), int(Qt::AlignRight)}) {
                const QByteArray name = QByteArray::number(start) + '-'
                                        + QByteArray::number(alignment);
                QTest::newRow(name.constData()) << start << alignment;
            }
        }
    }

    void selectiveExport()
    {
        QFETCH(int, start);
        QFETCH(int, alignment);
        QTextDocument document;
        configureDocument(document, start, Qt::Alignment(alignment));
        TitlerGradientV1::applyBrushes(&document, 600, int(document.size().height()));
        const QImage expected = renderDocument(document);
        QVERIFY(coversGradient(expected));
        Mlt::Profile profile("atsc_720p_25");
        Mlt::Producer producer(profile, "kdenlivetitle", "");
        QVERIFY(producer.is_valid());
        setTitle(producer, titleXml(document, Qt::Alignment(alignment)));
        const QImage exported = renderFrame(producer, 240);
        QVERIFY(coversGradient(exported));
        QCOMPARE(exported, expected);
    }

    void suffixDeletionAndPersistence()
    {
        QTextDocument document;
        configureDocument(document, 0);
        TitlerGradientV1::applyBrushes(&document, 600, int(document.size().height()));
        const QImage before = coloredPixels(renderDocument(document));
        QTextCursor cursor(&document);
        cursor.setPosition(4);
        cursor.setPosition(document.characterCount() - 1, QTextCursor::KeepAnchor);
        cursor.removeSelectedText();
        document.setTextWidth(-1);
        const QSizeF size = document.size();
        TitlerGradientV1::applyBrushes(&document, int(size.width()), int(size.height()));
        const QImage after = coloredPixels(renderDocument(document));
        QVERIFY(coversGradient(after));
        QCOMPARE(after, before);
        QDomDocument xml;
        auto content = xml.createElement(QStringLiteral("content"));
        xml.appendChild(content);
        content.appendChild(TitlerGradientV1::save(xml, &document));
        QTextDocument restored;
        restored.setHtml(document.toHtml());
        restored.setDocumentMargin(0);
        restored.setTextWidth(-1);
        QVERIFY(TitlerGradientV1::restore(content, &restored));
        const QSizeF restoredSize = restored.size();
        TitlerGradientV1::applyBrushes(&restored,
                                       int(restoredSize.width()),
                                       int(restoredSize.height()));
        QCOMPARE(renderDocument(restored), renderDocument(document));
    }

    void typewriterKeepsFullRegionCoordinates()
    {
        QTextDocument document;
        configureDocument(document, 0, Qt::AlignRight);
        TitlerGradientV1::applyBrushes(&document, 600, int(document.size().height()));
        Mlt::Profile profile("atsc_720p_25");
        Mlt::Producer reference(profile, "kdenlivetitle", "");
        Mlt::Producer native(profile, "kdenlivetitle", "");
        Mlt::Producer external(profile, "kdenlivetitle", "");
        QVERIFY(reference.is_valid());
        QVERIFY(native.is_valid());
        QVERIFY(external.is_valid());
        const auto staticXml = titleXml(document, Qt::AlignRight);
        setTitle(reference, staticXml);
        setTitle(native, titleXml(document, Qt::AlignRight, true));
        setTitle(external, staticXml);
        Mlt::Filter effect(profile, "typewriter");
        QVERIFY(effect.is_valid());
        effect.set("macro_type", 1);
        effect.set("step_length", 2);
        effect.set("step_sigma", 0);
        effect.set("random_seed", 37);
        QCOMPARE(external.attach(effect), 0);
        const QImage finalImage = renderFrame(reference, 240);
        QVERIFY(coversGradient(finalImage));
        for (Mlt::Producer *animated : {&native, &external}) {
            const QByteArray xmlBefore(animated->get("xmldata"));
            const QImage early = renderFrame(*animated, 0);
            QVERIFY(!early.isNull());
            QVERIFY(early != finalImage);
            int interiors = 0;
            for (int y = 0; y < early.height(); ++y) {
                for (int x = 0; x < early.width(); ++x) {
                    const QColor color = early.pixelColor(x, y);
                    if (color.alpha() == 255) {
                        QCOMPARE(color, finalImage.pixelColor(x, y));
                        ++interiors;
                    }
                }
            }
            QVERIFY(interiors > 20);
            QCOMPARE(renderFrame(*animated, 240), finalImage);
            QCOMPARE(renderFrame(*animated, 0), early);
            QCOMPARE(QByteArray(animated->get("xmldata")), xmlBefore);
        }
    }

    void legacyObjectGradientTakesPrecedence()
    {
        QTextDocument document;
        configureDocument(document, 5, Qt::AlignRight);
        const auto xmlBefore = titleXml(document, Qt::AlignRight);
        QDomDocument xml(xmlBefore);
        auto content = xml.documentElement()
                           .firstChildElement(QStringLiteral("item"))
                           .firstChildElement(QStringLiteral("content"));
        content.setAttribute(QStringLiteral("gradient"), recipe);
        QTextCursor cursor(&document);
        cursor.select(QTextCursor::Document);
        QTextCharFormat legacy;
        legacy.setForeground(QBrush(
            TitlerGradientV1::gradientFromString(recipe, 600, int(document.size().height()))));
        cursor.mergeCharFormat(legacy);
        const QImage expected = renderDocument(document);
        Mlt::Profile profile("atsc_720p_25");
        Mlt::Producer producer(profile, "kdenlivetitle", "");
        QVERIFY(producer.is_valid());
        setTitle(producer, xml);
        QCOMPARE(renderFrame(producer, 240), expected);
    }
};

QTEST_MAIN(TestRichTextGradient)

#include "test_richtextgradient.moc"
