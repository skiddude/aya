/**
 *  GalleryItemColor.h
 */

#pragma once

#define signals protected
#define slots
#include <QtnRibbonGallery.h>
#undef signals
#undef slots

#include "Utility/BrickColor.hpp"


#define GALLERY_ITEM_USER_DATA 100

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class StudioGalleryItem : public Qtitan::RibbonGalleryItem
{
public:
    StudioGalleryItem() {}

protected:
    virtual void draw(QPainter* pPainter, Qtitan::RibbonGallery* pGallery, QRect rectItem, bool enabled, bool selected, bool pressed, bool checked);
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class GalleryItemColor : public Qtitan::RibbonGalleryItem
{
public:
    static void addStandardColors(Qtitan::RibbonGalleryGroup* items);

    QColor getColor() const
    {
        return m_color;
    }
    Aya::BrickColor getBrickColor() const
    {
        return m_brickColor;
    }

private:
    GalleryItemColor(const Aya::BrickColor& color);
    virtual void draw(QPainter* pPainter, Qtitan::RibbonGallery* pGallery, QRect rectItem, bool enabled, bool selected, bool pressed, bool checked);

    QColor m_color;
    Aya::BrickColor m_brickColor;
};