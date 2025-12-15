

#pragma once

// Qt Headers
#include <QListWidget>
#include <QTextBrowser>

// Roblox Headers
#include "Reflection/EnumConverter.hpp"

// Roblox Studio Headers
#include "RobloxBasicDoc.hpp"

class QSplitter;

namespace Aya
{
namespace Reflection
{
class Descriptor;
class ClassDescriptor;
class PropertyDescriptor;
class FunctionDescriptor;
class YieldFunctionDescriptor;
class EventDescriptor;
class CallbackDescriptor;
namespace Metadata
{
class Reflection;
}
} // namespace Reflection
} // namespace Aya

class ObjectBrowserItem : public QListWidgetItem
{
public:
    ObjectBrowserItem(const Aya::Reflection::Descriptor* desc);
    const Aya::Reflection::Descriptor* getDescriptor()
    {
        return m_descriptor;
    }

private:
    const Aya::Reflection::Descriptor* m_descriptor;
};

class DeclarationView : public QTextBrowser
{
public:
    DeclarationView(QWidget* parent);

    void updateDeclarationView(ObjectBrowserItem* item);

private:
    void constructView(std::string& decl, std::string& owner, bool backend, bool bRestricted, bool bDeprecated, std::string& summary);
    const Aya::Reflection::Descriptor* m_descriptor;
};

class MemberListWidget : public QListWidget
{
public:
    MemberListWidget(QWidget* parent);

    void updateMemberList(ObjectBrowserItem* item);

private:
    void insertProperty(const Aya::Reflection::PropertyDescriptor* d);
    void insertFunction(const Aya::Reflection::FunctionDescriptor* d);
    void insertYieldFunction(const Aya::Reflection::YieldFunctionDescriptor* d);
    void insertSignal(const Aya::Reflection::EventDescriptor* d);
    void insertCallback(const Aya::Reflection::CallbackDescriptor* d);
    void insertEnumItem(const Aya::Reflection::EnumDescriptor::Item* e);

    std::vector<ObjectBrowserItem*> m_MemberItemVector;
    const Aya::Reflection::Descriptor* m_descriptor;
    bool m_showInheritedMembers;

    int toBeDeleted_Cnt;
};

class ClassListWidget : public QListWidget
{
public:
    ClassListWidget(QWidget* parent);

    void init();
    QSize sizeHint() const;

private:
    void insertClass(const Aya::Reflection::ClassDescriptor* d);
    void insertEnum(const Aya::Reflection::EnumDescriptor* enumDesc);

    std::vector<ObjectBrowserItem*> m_ClassItemVector;
};

class RobloxObjectBrowserDoc
    : public QObject
    , public RobloxBasicDoc
{
    Q_OBJECT

public:
    RobloxObjectBrowserDoc();
    ~RobloxObjectBrowserDoc();

    bool open(RobloxMainWindow* pMainWindow, const QString& fileName = "");

    IRobloxDoc::RBXCloseRequest requestClose()
    {
        return IRobloxDoc::NO_SAVE_NEEDED;
    }

    IRobloxDoc::RBXDocType docType()
    {
        return IRobloxDoc::OBJECTBROWSER;
    }

    QString fileName() const
    {
        return "";
    }
    QString displayName() const
    {
        return m_displayName;
    }
    QString keyName() const
    {
        return "ObjectBrowser";
    }

    bool save()
    {
        return false;
    }
    bool saveAs(const QString&)
    {
        return false;
    }

    QString saveFileFilters()
    {
        return "";
    }

    QWidget* getViewer()
    {
        return m_ObjectBrowser;
    }

    bool isModified()
    {
        return false;
    }

    void activate();
    void deActivate();

    bool actionState(const QString& actionID, bool& enableState, bool& checkedState);

    bool handlePluginAction(void*, void*)
    {
        return false;
    }

    void handleScriptCommand(const QString&)
    {
        return;
    }

    bool supportsZeroPlaneGrid()
    {
        return false;
    }

private Q_SLOTS:
    void onClassSelected(QListWidgetItem* current, QListWidgetItem* previous);
    void onMemberSelected(QListWidgetItem* current, QListWidgetItem* previous);

private:
    virtual bool doClose();
    void initializeAllViews();
    void clearListWidget(QListWidget* listWidget);

    std::vector<ObjectBrowserItem*> m_MemberItemVector;

    QWidget* m_ObjectBrowser;
    QSplitter* m_vSplitter;
    QSplitter* m_hSplitter;
    ClassListWidget* m_ClassList;
    MemberListWidget* m_MemberList;
    DeclarationView* m_DeclarationView;
    QString m_displayName;

    bool state;

    static int sOBCount;
};
