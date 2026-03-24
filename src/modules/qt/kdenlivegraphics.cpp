#include "kdenlivegraphics.h"

KdenliveGraphicsRect::KdenliveGraphicsRect(QRectF rect,
                                           const QBrush &brush,
                                           const QPen &pen,
                                           int cornerRadius)
{
    this->setRect(rect);
    this->setBrush(brush);
    this->setPen(pen);
    this->cornerRadius = cornerRadius;
}

void KdenliveGraphicsRect::paint(QPainter *painter,
                                 const QStyleOptionGraphicsItem *option,
                                 QWidget *widget)
{
    int radius = qMin(cornerRadius, (int) qMin(rect().width(), rect().height()) / 2);
    painter->setBrush(brush());
    painter->setPen(pen());
    painter->setRenderHint(QPainter::Antialiasing);
    painter->drawRoundedRect(rect(), radius, radius);
}
