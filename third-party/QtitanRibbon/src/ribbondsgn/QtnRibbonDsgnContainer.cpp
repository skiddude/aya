#include <QAction>
#include <qevent.h>

#include <QtDesigner/QExtensionManager>
#include <QtDesigner/QDesignerFormWindowInterface>
#include <QtDesigner/QDesignerFormEditorInterface>
#include <QtDesigner/QDesignerMetaDataBaseInterface>
#include <QtDesigner/QDesignerWidgetFactoryInterface>

#include "QtnRibbonDsgnPlugins.h"
#include "QtnRibbonDsgnContainer.h"
#include "QtnRibbonBar.h"
#include "QtnRibbonPage.h"
#include "../ribbon/QtnRibbonTabBar.h"

QTITAN_USE_NAMESPACE


static void setObjectNameChildren(RibbonTabBar& widget)
{
    QList<RibbonTab*> child_list = widget.findChildren<RibbonTab*>();
    for(int i = 0, count = child_list.count(); count > i; i++)
    {
        if (RibbonTab* pWd = child_list[i])
            pWd->setObjectName("__qt__passive_RibbonTab");
    }
}


/* RibbonTabBarFilter */
RibbonTabBarFilter* RibbonTabBarFilter::tf = Q_NULL;
RibbonTabBarFilter::RibbonTabBarFilter(RibbonTabBar* pWidget)
    : QObject(pWidget)
{
    m_ribbonTabBar = pWidget;
    m_ribbonTabBar->setObjectName("__qt__passive_RibbonTabBar");
}

RibbonTabBarFilter::~RibbonTabBarFilter()
{
    uninstall(m_ribbonTabBar, false);
}

void RibbonTabBarFilter::install(QWidget* pWidget)
{
    if (!tf)
    {
        RibbonTabBar* pRibbonTabBar = pWidget->findChild<RibbonTabBar*>();
        Q_ASSERT(pRibbonTabBar != Q_NULL);

        tf = new RibbonTabBarFilter(pRibbonTabBar);

        pRibbonTabBar->installEventFilter(tf);
        pRibbonTabBar->setMouseTracking(true);
    }
}

void RibbonTabBarFilter::uninstall(QWidget* pWidget, bool deleteThis)
{
    if (RibbonTabBarFilter::tf)
    {
        if (deleteThis)
        {
            RibbonTabBar* pRibbonTabBar = pWidget->findChild<RibbonTabBar*>();
            Q_ASSERT(pRibbonTabBar != Q_NULL);
            pRibbonTabBar->removeEventFilter(RibbonTabBarFilter::tf);
        }
        else if (RibbonTabBarFilter::tf->m_ribbonTabBar)
            RibbonTabBarFilter::tf->m_ribbonTabBar->removeEventFilter(RibbonTabBarFilter::tf);

        if (deleteThis)
            delete RibbonTabBarFilter::tf;
        RibbonTabBarFilter::tf = Q_NULL;
    }
}

bool RibbonTabBarFilter::eventFilter(QObject* watched, QEvent* event)
{
    if (watched != m_ribbonTabBar)
        return QObject::eventFilter(watched, event);

    switch (event->type()) 
    {
        case QEvent::ContextMenu:
            return m_ribbonTabBar->parentWidget()->eventFilter(watched, event);
        default:
            break;
    }
    return QObject::eventFilter(watched, event);
}

void DsgnRibbonBarContainer::addWidget(QWidget* widget)
{
    if (RibbonPage* ribbonPage = qobject_cast<RibbonPage*>(widget))
    {
        if (ribbonPage->title().isEmpty())
        {
            QString title = tr("Page %1").arg(count());
            ribbonPage->setTitle(title);
        }

        m_pRibbon->addPage(ribbonPage);
        ribbonPage->setAutoFillBackground(false);

        if (RibbonTabBar* ribbonTabBar = m_pRibbon->findChild<RibbonTabBar*>())
            setObjectNameChildren(*ribbonTabBar);

        if (QDesignerFormWindowInterface* formWindowInterface =  QDesignerFormWindowInterface::findFormWindow(m_pRibbon))
            formWindowInterface->manageWidget(ribbonPage);

        // set current page
        m_pRibbon->setCurrentPageIndex(count() - 1);
    }
}

void DsgnRibbonBarContainer::insertWidget(int index, QWidget* widget)
{
    if (RibbonPage* ribbonPage = qobject_cast<RibbonPage*>(widget))
    {
        if (ribbonPage->title().isEmpty())
        {
            QString title = tr("Page %1").arg(count());
            ribbonPage->setTitle(title);
        }

        m_pRibbon->insertPage(index, ribbonPage);
        ribbonPage->setAutoFillBackground(false);

        if (RibbonTabBar* ribbonTabBar = m_pRibbon->findChild<RibbonTabBar*>())
            setObjectNameChildren(*ribbonTabBar);

        if (QDesignerFormWindowInterface* formWindowInterface = QDesignerFormWindowInterface::findFormWindow(m_pRibbon))
            formWindowInterface->manageWidget(ribbonPage);

        // set current page
        m_pRibbon->setCurrentPageIndex(index);
    }
}

void DsgnRibbonBarContainer::remove(int index)
{
    m_pRibbon->detachPage(index);
}

/* DsgnRibbonBarContainerFactory */
DsgnRibbonBarContainerFactory::DsgnRibbonBarContainerFactory(QExtensionManager* parent)
    : QExtensionFactory(parent)
{
}

QObject* DsgnRibbonBarContainerFactory::createExtension(QObject* object, const QString& iid, QObject* parent) const
{
    RibbonBar* widget = qobject_cast<RibbonBar*>(object);

    if (widget && (iid == Q_TYPEID(QDesignerContainerExtension))) 
        return new DsgnRibbonBarContainer(widget, parent);
    else
        return Q_NULL;
}