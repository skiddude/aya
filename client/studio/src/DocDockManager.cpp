

#include "DocDockManager.hpp"

// Qt Headers
#include <QVBoxLayout>
#include <QToolBar>

// Roblox Studio Headers
#include "RobloxMainWindow.hpp"
#include "RobloxDocManager.hpp"
#include "StudioUtilities.hpp"
#include "DocDockWidget.hpp"

DocDockManager::DocDockManager(RobloxMainWindow& mainWindow)
    : QObject(&mainWindow)
    , m_MainWindow(mainWindow)
{
}

DocDockManager::~DocDockManager()
{
    AYAASSERT(m_Docs.isEmpty());
}

/**
 * Adds a new dock widget for a doc.
 */
void DocDockManager::addDoc(IRobloxDoc& doc)
{
    AYAASSERT(!m_Docs.contains(&doc));
    AYAASSERT(doc.getViewer());

    Aya::BaldPtr<DocDockWidget> dockWidget = new DocDockWidget(m_MainWindow, *this, doc);
    m_Docs[&doc] = dockWidget;
}

/**
 * Removes the dock widget for the doc but does not delete the doc.
 *
 * @return false if the doc is not handled by this manager.
 */
bool DocDockManager::removeDoc(IRobloxDoc& doc)
{
    if (!m_Docs.contains(&doc))
        return false;

    Aya::BaldPtr<DocDockWidget> dockWidget = m_Docs[&doc];
    dockWidget->cleanup();
    dockWidget->deleteLater();
    m_Docs.remove(&doc);
    return true;
}

/**
 * Changes the dock widget's window title.
 *
 * @return false if the doc is not handled by this manager.
 */
bool DocDockManager::renameDoc(IRobloxDoc& doc, const QString& text, const QString& tooltip)
{
    if (!m_Docs.contains(&doc))
        return false;

    Aya::BaldPtr<DocDockWidget> dockWidget = m_Docs[&doc];
    dockWidget->setTitle(text, tooltip);
    return true;
}

/**
 * Sets dock widget focus for the doc, causing the doc to be set current.
 *
 * @return false if the doc is not handled by this manager.
 */
bool DocDockManager::setCurrentDoc(IRobloxDoc& doc)
{
    if (!m_Docs.contains(&doc))
        return false;

    Aya::BaldPtr<DocDockWidget> dockWidget = m_Docs[&doc];
    dockWidget->setFocus();
    dockWidget->raise();
    return true;
}

/**
 * Gets the current dock widget that has focus.
 *
 * @return doc if has focus, NULL if not found or does not have focus
 */
IRobloxDoc* DocDockManager::getCurrentDoc() const
{
    AYAASSERT(QThread::currentThread() == QCoreApplication::instance()->thread());

    tDocDockMap::const_iterator iter = m_Docs.begin();
    while (iter != m_Docs.end())
    {
        if (iter.value()->hasFocus())
            return iter.key();
        ++iter;
    }

    return NULL;
}

/**
 * Initializes dragging a dock widget.
 *  This is called when a tab is detached, creating a dock widget.  The dock widget
 *   is immediately set to be dragging the window.
 *  This function generates OS specific mouse events.
 */
void DocDockManager::startDrag(IRobloxDoc& doc)
{
    AYAASSERT(m_Docs.contains(&doc));
    Aya::BaldPtr<DocDockWidget> dockWidget = m_Docs[&doc];
    dockWidget->startDragging();
}
