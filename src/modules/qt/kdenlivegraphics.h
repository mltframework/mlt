#pragma once

#include <QBrush>
#include <QGraphicsRectItem>
#include <QPainter>
#include <QPen>

class KdenliveGraphicsRect : public QGraphicsRectItem
{
public:
    explicit KdenliveGraphicsRect(QRectF rect,
                                  const QBrush &brush,
                                  const QPen &pen,
                                  int cornerRadius);

protected:
    void paint(QPainter *painter,
               const QStyleOptionGraphicsItem *option,
               QWidget *widget = nullptr) override;

private:
    int cornerRadius{0};
};
