#if defined(__linux) || defined(__APPLE__)
#include <QPainter>
#include "QtnCommonStylePrivate.h"
#include "QtnRibbonStylePrivate.h"
#include "QtnRibbonMainWindow.h"
#include "QtnStyleHelpers.h"
#ifdef DEBUG_MEMORY_ENABLED
#include "QtitanMSVSDebug.h"
#endif

QTITAN_USE_NAMESPACE


#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))

enum HBitmapFormat 
{
    HBitmapNoAlpha,
    HBitmapPremultipliedAlpha,
    HBitmapAlpha
};
#endif

QPixmap StyleHelper::alphaBlend(const QPixmap& src)
{
//    HBITMAP winBitmap1 = src.toWinHBITMAP(QPixmap::Alpha);
//    return QPixmap::fromWinHBITMAP(winBitmap1, QPixmap::PremultipliedAlpha);

    return src;
}

// for QForm
/*! \internal */
bool RibbonPaintManager2013::drawFrame(const QStyleOption* opt, QPainter* p, const QWidget* w) const
{
#ifdef Q_OS_WIN
    Q_UNUSED(p);
    QTN_D_STYLE(RibbonStyle)
    if (qobject_cast<const RibbonMainWindow*>(w))
    {
        if (const StyleOptionFrame* optFrame = qstyleoption_cast<const StyleOptionFrame*>(opt))
        {
            HDC hdc = (HDC)optFrame->hdc;

            QRect rc = optFrame->rect;
            rc.adjust(0, 0, -1, -1);

            QRect rcBorders = optFrame->clientRect;
            int nRightBorder = rcBorders.left() - rc.left(), nLeftBorder = rcBorders.left() - rc.left(), nTopBorder = rcBorders.top() - rc.top();
            int nBottomBorder = rc.bottom() - rcBorders.bottom() + DrawHelpers::dpiScaled(2);

            HBRUSH hBrush = ::CreateSolidBrush(rgbcolorref(d.m_clrRibbonFace));
            Q_ASSERT(hBrush != Q_NULL);

            // draw top
            RECT rectTop = {0, 0, rc.width(), nTopBorder};
            ::FillRect(hdc, &rectTop, hBrush);

            // draw left
            RECT rectLeft = {0, 0, nLeftBorder, rc.height()};
            ::FillRect(hdc, &rectLeft, hBrush);

            // draw right
            RECT rectRight = {rc.width() - nRightBorder, 0, rc.width() + nRightBorder, rc.height()};
            ::FillRect(hdc, &rectRight, hBrush);

            // draw bottom
            RECT rectBottom = {0, rc.height() - nBottomBorder, rc.width(), rc.height() + nBottomBorder};
            ::FillRect(hdc, &rectBottom, hBrush);

            ::DeleteObject(hBrush);

            COLORREF clrBorder = optFrame->active ? rgbcolorref(d.m_clrFrameBorderActive0) : rgbcolorref(d.m_clrFrameBorderInactive0);

            HPEN hPen = ::CreatePen(PS_SOLID, 1, clrBorder);
            HGDIOBJ hOldPen = ::SelectObject (hdc, (HGDIOBJ)hPen);

            ::LineTo(hdc, 0, 0); ::LineTo(hdc, rc.width(), 0 );
            ::LineTo(hdc, rc.width()-1, 0); ::LineTo(hdc, rc.width()-1, rc.height() );
            ::LineTo(hdc, rc.width(), rc.height()-1); ::LineTo(hdc, 0, rc.height()-1 );
            ::LineTo(hdc, 0, rc.height()-1); ::LineTo(hdc, 0, 0 );

            SelectObject(hdc, hOldPen);
            ::DeleteObject(hPen);

            if (optFrame->hasStatusBar && !optFrame->isBackstageVisible)
            {
                int statusHeight = optFrame->statusHeight;
                HBRUSH hBrushStatusBar = ::CreateSolidBrush(rgbcolorref(d.m_clrStatusBarShadow));
                Q_ASSERT(hBrushStatusBar != Q_NULL);

                RECT rectBottom = {0, rc.height() - nBottomBorder, rc.width(), rc.height() + nBottomBorder};
                ::FillRect(hdc, &rectBottom, hBrushStatusBar);

                RECT rectLeft = {0, rc.height() - (statusHeight + (int)DrawHelpers::dpiScaled(nTopBorder)), nLeftBorder, rc.height() - nBottomBorder};
                ::FillRect(hdc, &rectLeft, hBrushStatusBar);

                RECT rectRight = {rc.width() - nRightBorder, rc.height() - (statusHeight + (int)DrawHelpers::dpiScaled(nTopBorder)), rc.width(), rc.height() - nBottomBorder};
                ::FillRect(hdc, &rectRight, hBrushStatusBar);

                ::DeleteObject(hBrushStatusBar);
            }
        }
        return true;
    }
#endif // Q_OS_WIN
    return false;
}

// for QForm
bool RibbonPaintManager2016::drawFrame(const QStyleOption* opt, QPainter* p, const QWidget* w) const
{
#ifdef Q_OS_WIN
    Q_UNUSED(p);
    QTN_D_STYLE(RibbonStyle)
    if (qobject_cast<const RibbonMainWindow*>(w))
    {
        if (const StyleOptionFrame* optFrame = qstyleoption_cast<const StyleOptionFrame*>(opt))
        {
            HDC hdc = (HDC)optFrame->hdc;

            QRect rc = optFrame->rect;
            rc.adjust(0, 0, -1, -1);

            OfficeStyle::Theme officeTheme = theme();
            QRect rcBorders = optFrame->clientRect;
            int nRightBorder = rcBorders.left() - rc.left(), nLeftBorder = rcBorders.left() - rc.left(), nTopBorder = rcBorders.top() - rc.top();
            int nBottomBorder = rc.bottom() - rcBorders.bottom() + DrawHelpers::dpiScaled(2);
            int nTopCaptionTabBar = optFrame->titleBarSize + (optFrame->tabBarPosition == RibbonBar::TabBarBottomPosition ? -nBottomBorder : optFrame->tabBarSize);

            COLORREF clr = rgbcolorref(optFrame->isBackstageVisible ? m_clrBarLight : d.m_clrRibbonFace);
            if (officeTheme == OfficeStyle::Office2016Black)
                clr = rgbcolorref(m_clrBarShadow);

            HBRUSH hBrushTop = ::CreateSolidBrush(clr);
            Q_ASSERT(hBrushTop != Q_NULL);

            HBRUSH hBrush = ::CreateSolidBrush(rgbcolorref(m_clrBarLight));
            Q_ASSERT(hBrush != Q_NULL);

            // draw top
            RECT rectTop = { 0, 0, rc.width(), nTopBorder };
            ::FillRect(hdc, &rectTop, hBrushTop);

            // draw leftTopCaptionTabBar
            RECT rectLeftTopCaptionTabBar = { 0, 0, nLeftBorder, nTopCaptionTabBar };
            ::FillRect(hdc, &rectLeftTopCaptionTabBar, hBrushTop);

            // draw left
            RECT rectLeft = { 0, nTopCaptionTabBar, nLeftBorder, rc.height() };
            ::FillRect(hdc, &rectLeft, hBrush);

            // draw rightTopCaptionTabBar
            RECT rectRightTopCaptionTabBar = { rc.width() - nRightBorder, 0, rc.width() + nRightBorder, nTopCaptionTabBar };
            ::FillRect(hdc, &rectRightTopCaptionTabBar, hBrushTop);

            // draw right
            RECT rectRight = { rc.width() - nRightBorder, nTopCaptionTabBar, rc.width() + nRightBorder, rc.height() };
            ::FillRect(hdc, &rectRight, hBrush);

            // draw bottom
            RECT rectBottom = { 0, rc.height() - nBottomBorder, rc.width(), rc.height() + nBottomBorder };
            ::FillRect(hdc, &rectBottom, hBrush);

            ::DeleteObject(hBrush);
            ::DeleteObject(hBrushTop);

            COLORREF clrBorder = optFrame->active ? rgbcolorref(d.m_clrFrameBorderActive0) : rgbcolorref(d.m_clrFrameBorderInactive0);

            HPEN hPen = ::CreatePen(PS_SOLID, 1, clrBorder);
            HGDIOBJ hOldPen = ::SelectObject(hdc, (HGDIOBJ)hPen);

            ::LineTo(hdc, 0, 0); ::LineTo(hdc, rc.width(), 0);
            ::LineTo(hdc, rc.width() - 1, 0); ::LineTo(hdc, rc.width() - 1, rc.height());
            ::LineTo(hdc, rc.width(), rc.height() - 1); ::LineTo(hdc, 0, rc.height() - 1);
            ::LineTo(hdc, 0, rc.height() - 1); ::LineTo(hdc, 0, 0);

            ::SelectObject(hdc, hOldPen);
            ::DeleteObject(hPen);

            if ((officeTheme == OfficeStyle::Office2016DarkGray || officeTheme == OfficeStyle::Office2016White || officeTheme == OfficeStyle::Office2016Black) 
                && optFrame->hasStatusBar && !optFrame->isBackstageVisible)
            {
                int statusHeight = optFrame->statusHeight;
                HBRUSH hBrushStatusBar = ::CreateSolidBrush(rgbcolorref(d.m_clrStatusBarShadow));
                Q_ASSERT(hBrushStatusBar != Q_NULL);

                RECT rectBottom = { 0, rc.height() - nBottomBorder, rc.width(), rc.height() + nBottomBorder };
                ::FillRect(hdc, &rectBottom, hBrushStatusBar);

                RECT rectLeft = { 0, rc.height() - (statusHeight + (int)DrawHelpers::dpiScaled(nTopBorder)), nLeftBorder, rc.height() - nBottomBorder };
                ::FillRect(hdc, &rectLeft, hBrushStatusBar);

                RECT rectRight = { rc.width() - nRightBorder, rc.height() - (statusHeight + (int)DrawHelpers::dpiScaled(nTopBorder)), rc.width(), rc.height() - nBottomBorder };
                ::FillRect(hdc, &rectRight, hBrushStatusBar);
                ::DeleteObject(hBrushStatusBar);

                if (officeTheme == OfficeStyle::Office2016Black && !optFrame->active)
                {
                    COLORREF clrBorder = rgbcolorref(d.m_clrFrameBorderInactive0);
                    hBrushStatusBar = ::CreateSolidBrush(clrBorder);
                    Q_ASSERT(hBrushStatusBar != Q_NULL);

                    RECT rectBottom = { 0, rc.height() - 1, rc.width(), rc.height() + 1 };
                    ::FillRect(hdc, &rectBottom, hBrushStatusBar);

                    RECT rectLeft = { 0, rc.height() - (statusHeight + (int)DrawHelpers::dpiScaled(nTopBorder)), 1, rc.height() - 1};
                    ::FillRect(hdc, &rectLeft, hBrushStatusBar);

                    RECT rectRight = { rc.width() - 1, rc.height() - (statusHeight + (int)DrawHelpers::dpiScaled(nTopBorder)), rc.width(), rc.height() - 1 };
                    ::FillRect(hdc, &rectRight, hBrushStatusBar);
                    ::DeleteObject(hBrushStatusBar);
                }
            }

        }
        return true;
    }
#endif // Q_OS_WIN
    return false;
}
#endif