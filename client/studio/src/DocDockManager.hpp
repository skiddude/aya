#pragma once

// Qt Headers
#include <QDockWidget>
#include <QMap>

// Roblox Headers
#include "BaldPtr.hpp"


// Roblox Studio Headers
#include "IRobloxDoc.hpp"

class RobloxMainWindow;
class DocDockWidget;

class DocDockManager : public QObject
{
    Q_OBJECT

public:
    DocDockManager(RobloxMainWindow& mainWindow);
    virtual ~DocDockManager();

    void addDoc(IRobloxDoc& doc);
    bool removeDoc(IRobloxDoc& doc);
    bool renameDoc(IRobloxDoc& doc, const QString& text, const QString& tooltip);

    bool setCurrentDoc(IRobloxDoc& doc);
    IRobloxDoc* getCurrentDoc() const;

    void startDrag(IRobloxDoc& doc);

Q_SIGNALS:

    void attachTab(IRobloxDoc& doc);

private:
    typedef QMap<Aya::BaldPtr<IRobloxDoc>, Aya::BaldPtr<DocDockWidget>> tDocDockMap;

    RobloxMainWindow& m_MainWindow;
    tDocDockMap m_Docs;
};
