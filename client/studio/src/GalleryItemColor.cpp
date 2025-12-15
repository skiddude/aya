/**
 *  GalleryItemColor.cpp
 */


#include "GalleryItemColor.hpp"

// Qt headers
#include <QPainter>
#include <QStyleOption>
#include <QtnStyleHelpers.h>

// Roblox headers
#include "Utility/BrickColor.hpp"


// Roblox Studio Headers
#include "QtUtilities.hpp"

void StudioGalleryItem::draw(
    QPainter* pPainter, Qtitan::RibbonGallery* pGallery, QRect rectItem, bool enabled, bool selected, bool pressed, bool checked)
{
    if (icon().isNull())
    {
        Qtitan::RibbonGalleryItem::draw(pPainter, pGallery, rectItem, enabled, selected, pressed, checked);
        return;
    }

    rectItem.adjust(2, 2, -2, -2);
    pPainter->setPen(QPen(Qt::gray, 1));
    pPainter->drawRect(rectItem);

    if (!enabled)
    {
        pPainter->drawPixmap(rectItem, icon().pixmap(rectItem.size(), QIcon::Disabled));
        return;
    }

    pPainter->drawPixmap(rectItem, icon().pixmap(rectItem.size()));

    if (selected || checked)
    {
        rectItem.adjust(0, 0, -1, -1);

        if (selected)
            Qtitan::DrawHelpers::draw3DRect(
                *pPainter, QColor(242, 148, 54), QColor(242, 148, 54), rectItem.left(), rectItem.top(), rectItem.width(), rectItem.height(), true);
        else
            Qtitan::DrawHelpers::draw3DRect(
                *pPainter, QColor(239, 72, 16), QColor(239, 72, 16), rectItem.left(), rectItem.top(), rectItem.width(), rectItem.height(), true);

        rectItem.adjust(1, 1, -1, -1);
        Qtitan::DrawHelpers::draw3DRect(
            *pPainter, QColor(255, 226, 148), QColor(255, 226, 148), rectItem.left(), rectItem.top(), rectItem.width(), rectItem.height(), true);
    }
}

void GalleryItemColor::addStandardColors(Qtitan::RibbonGalleryGroup* pGalleryGroup)
{
    std::vector<Aya::BrickColor>::const_iterator iter = Aya::BrickColor::colorPalette().begin();
    while (iter != Aya::BrickColor::colorPalette().end())
    {
        GalleryItemColor* pItem = new GalleryItemColor(*iter);
        pItem->setSizeHint(pGalleryGroup->size());
        pItem->setToolTip(iter->name().c_str());
        pItem->setData(GALLERY_ITEM_USER_DATA, iter->number);

        pGalleryGroup->appendItem(pItem);
        iter++;
    }
}

GalleryItemColor::GalleryItemColor(const Aya::BrickColor& color)
    : m_brickColor(color)
{
    m_color = QtUtilities::toQColor(color.color3());
}

void GalleryItemColor::draw(
    QPainter* pPainter, Qtitan::RibbonGallery* pGallery, QRect rectItem, bool enabled, bool selected, bool pressed, bool checked)
{
    // set margin
    rectItem.adjust(2, 2, -2, -2);
    pPainter->setPen(QPen(Qt::black, 1));
    pPainter->drawRect(rectItem);

    if (!enabled)
    {
        // draw disabled
        int grayScale = qGray(m_color.rgb());
        pPainter->fillRect(rectItem, QColor(grayScale, grayScale, grayScale));
        return;
    }

    // fill the item color
    pPainter->fillRect(rectItem, m_color);

    if (selected || checked)
    {
        rectItem.adjust(0, 0, -1, -1);

        if (selected)
            Qtitan::DrawHelpers::draw3DRect(
                *pPainter, QColor(242, 148, 54), QColor(242, 148, 54), rectItem.left(), rectItem.top(), rectItem.width(), rectItem.height(), true);
        else
            Qtitan::DrawHelpers::draw3DRect(
                *pPainter, QColor(239, 72, 16), QColor(239, 72, 16), rectItem.left(), rectItem.top(), rectItem.width(), rectItem.height(), true);

        rectItem.adjust(1, 1, -1, -1);
        Qtitan::DrawHelpers::draw3DRect(
            *pPainter, QColor(255, 226, 148), QColor(255, 226, 148), rectItem.left(), rectItem.top(), rectItem.width(), rectItem.height(), true);
    }
    else
    {
        pPainter->fillRect(rectItem.left(), rectItem.top(), rectItem.width(), 1, m_color);
        pPainter->fillRect(rectItem.left(), rectItem.top(), 1, rectItem.height(), m_color);
        pPainter->fillRect(rectItem.right(), rectItem.top(), 1, rectItem.height(), m_color);
        pPainter->fillRect(rectItem.left(), rectItem.bottom(), rectItem.width(), 1, m_color);
    }
}
