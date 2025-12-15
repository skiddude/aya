#pragma once

#include <QWindow>
#include <QEvent>
#include <QPoint>

class GfxView : public QWindow
{
private:
    QPoint lastMousePosition;
    QPoint lastMousePositionScaled;
};