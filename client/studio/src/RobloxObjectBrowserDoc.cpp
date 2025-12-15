


#include "RobloxObjectBrowserDoc.hpp"

// Qt Headers
#include <QSplitter>

// Roblox Headers
#include "Reflection/ReflectionMetadata.hpp"

// Roblox Studio Headers
#include "RobloxTreeWidget.hpp"
#include "RobloxMainWindow.hpp"
#include "AuthoringSettings.hpp"
#include "QtUtilities.hpp"
#include "UpdateUIManager.hpp"

#define OBJECT_BROWSER_IMAGES ":/images/img_classtree.bmp"
#define OB_IMAGE_SIZE 16

static int typeToInt(const Aya::Reflection::Descriptor* m)
{
    if (dynamic_cast<const Aya::Reflection::EnumDescriptor::Item*>(m))
        return 6;
    if (dynamic_cast<const Aya::Reflection::EnumDescriptor*>(m))
        return 5;
    if (dynamic_cast<const Aya::Reflection::EventDescriptor*>(m))
        return 4;
    if (dynamic_cast<const Aya::Reflection::CallbackDescriptor*>(m))
        return 3;
    if (dynamic_cast<const Aya::Reflection::PropertyDescriptor*>(m))
        return 2;
    if (dynamic_cast<const Aya::Reflection::FunctionDescriptor*>(m))
        return 1;
    if (dynamic_cast<const Aya::Reflection::ClassDescriptor*>(m))
        return 0;
    return 100;
}

template<bool includeType>
static void catenateArg(int& count, std::string& text, const Aya::Reflection::SignatureDescriptor::Item& item)
{
    if (count > 0)
        text += ", ";
    if (includeType)
    {
        text += item.type->name.c_str();
        text += " ";
    }

    text += item.name->c_str();

    if (includeType)
        if (item.defaultValue.type() == *item.type)
        {
            // A default value has been defined
            text += " = ";
            text += item.defaultValue.get<std::string>();
        }
    ++count;
}

void decorate(ObjectBrowserItem* item, const Aya::Reflection::Descriptor* desc)
{
    if (!desc)
        return;

    item->setText(desc->name.c_str());

    QFont font = item->font();
    Aya::Reflection::Metadata::Item* metadata = NULL;
    QIcon icon;
    switch (typeToInt(desc))
    {
    case 0: // ClassDescriptor
    {
        const Aya::Reflection::ClassDescriptor* d = dynamic_cast<const Aya::Reflection::ClassDescriptor*>(desc);
        metadata = Aya::Reflection::Metadata::Reflection::singleton()->get(*d, false);
        icon.addPixmap(QtUtilities::getPixmap(OBJECT_BROWSER_IMAGES, 3, OB_IMAGE_SIZE, true));
    }
    break;
    case 1: // Function Descriptor
    {
        const Aya::Reflection::FunctionDescriptor* d = dynamic_cast<const Aya::Reflection::FunctionDescriptor*>(desc);
        const Aya::Reflection::SignatureDescriptor& signature(d->getSignature());
        std::string text = d->name.toString();
        text += "(";
        int count = 0;
        std::for_each(
            signature.arguments.begin(), signature.arguments.end(), boost::bind(&catenateArg<false>, boost::ref(count), boost::ref(text), _1));
        text += ")";

        item->setText(text.c_str());
    }
    break;
    case 2: // Property Descriptor
    {
        const Aya::Reflection::PropertyDescriptor* d = dynamic_cast<const Aya::Reflection::PropertyDescriptor*>(desc);
        metadata = Aya::Reflection::Metadata::Reflection::singleton()->get(*d);
    }
    break;
    case 3: // Callback Descriptor
    {
        const Aya::Reflection::CallbackDescriptor* d = dynamic_cast<const Aya::Reflection::CallbackDescriptor*>(desc);
        metadata = Aya::Reflection::Metadata::Reflection::singleton()->get(*d);
    }
    break;
    case 4: // Event Descriptor
    {
        const Aya::Reflection::EventDescriptor* d = dynamic_cast<const Aya::Reflection::EventDescriptor*>(desc);
        metadata = Aya::Reflection::Metadata::Reflection::singleton()->get(*d);
    }
    break;
    case 5: // Enum Descriptor
    {
        item->setText(desc->name.c_str());
        icon.addPixmap(QtUtilities::getPixmap(OBJECT_BROWSER_IMAGES, 9, OB_IMAGE_SIZE, true));
    }
    break;
    case 6: // Enum Item Descriptor
        break;
    default:
        break;
    };

    if (Aya::Reflection::Metadata::Item::isDeprecated(metadata, *desc))
    {
        font.setStrikeOut(true);
    }

    item->setFont(font);
    item->setIcon(icon);
}

enum IconType
{
    FunctionIcon,
    PropertyIcon,
    SignalIcon
};

static int SecurityToIndex(IconType type, Aya::Security::Permissions security)
{
    switch (security)
    {
    case Aya::Security::None:
    {
        switch (type)
        {
        case FunctionIcon:
            return 4;
        case PropertyIcon:
            return 6;
        case SignalIcon:
            return 11;
        }
        return 0;
    }
    case Aya::Security::RobloxPlace:
    {
        switch (type)
        {
        case FunctionIcon:
            return 13;
        case PropertyIcon:
            return 14;
        case SignalIcon:
            return 15;
        }
        return 0;
    }
    default:
    {
        switch (type)
        {
        case FunctionIcon:
            return 5;
        case PropertyIcon:
            return 7;
        case SignalIcon:
            return 12;
        }
        return 0;
    }
    }
}

static bool compareFunc(ObjectBrowserItem* item1, ObjectBrowserItem* item2)
{
    const Aya::Reflection::Descriptor* m1 = item1->getDescriptor();
    const Aya::Reflection::Descriptor* m2 = item2->getDescriptor();

    int i1 = typeToInt(m1);
    int i2 = typeToInt(m2);
    if (i1 != i2)
        return i1 < i2;

    return item1->text() < item2->text();
}

RobloxObjectBrowserDoc::RobloxObjectBrowserDoc()
    : m_ObjectBrowser(NULL)
    , m_vSplitter(NULL)
    , m_hSplitter(NULL)
    , m_ClassList(NULL)
    , m_MemberList(NULL)
    , m_DeclarationView(NULL)
    , m_displayName(QString("Object Browser"))
{
}

RobloxObjectBrowserDoc::~RobloxObjectBrowserDoc()
{
    m_ClassList->deleteLater();
    m_MemberList->deleteLater();
    m_DeclarationView->deleteLater();
    m_ObjectBrowser->deleteLater();
}

bool RobloxObjectBrowserDoc::open(RobloxMainWindow* pMainWindow, const QString&)
{
    bool success = false;
    try
    {
        if (m_ObjectBrowser == NULL)
        {
            m_ObjectBrowser = new QWidget(pMainWindow);

            m_ClassList = new ClassListWidget(m_ObjectBrowser);
            m_MemberList = new MemberListWidget(m_ObjectBrowser);
            m_DeclarationView = new DeclarationView(m_ObjectBrowser);

            m_ClassList->setFrameStyle(QFrame::NoFrame);
            m_MemberList->setFrameStyle(QFrame::NoFrame);

            connect(m_ClassList, SIGNAL(currentItemChanged(QListWidgetItem*, QListWidgetItem*)), this,
                SLOT(onClassSelected(QListWidgetItem*, QListWidgetItem*)));
            connect(m_MemberList, SIGNAL(currentItemChanged(QListWidgetItem*, QListWidgetItem*)), this,
                SLOT(onMemberSelected(QListWidgetItem*, QListWidgetItem*)));

            m_vSplitter = new QSplitter(Qt::Vertical, m_ObjectBrowser);
            m_vSplitter->addWidget(m_MemberList);
            m_vSplitter->addWidget(m_DeclarationView);

            m_hSplitter = new QSplitter(m_ObjectBrowser);
            m_hSplitter->addWidget(m_ClassList);
            m_hSplitter->addWidget(m_vSplitter);
            m_hSplitter->setStretchFactor(0, 0);

            QVBoxLayout* layout = new QVBoxLayout;
            layout->setContentsMargins(0, 0, 0, 0);
            layout->setSpacing(0);
            layout->addWidget(m_hSplitter);
            m_ObjectBrowser->setLayout(layout);

            initializeAllViews();
        }

        success = true;
    }
    catch (...)
    {
        success = false;
    }
    return success;
}

bool RobloxObjectBrowserDoc::doClose()
{
    deActivate();
    return true;
}

void RobloxObjectBrowserDoc::activate()
{
    if (m_bActive)
        return;

    // update toobars
    UpdateUIManager::Instance().updateToolBars();

    m_bActive = true;
}


void RobloxObjectBrowserDoc::deActivate()
{
    if (!m_bActive)
        return;

    m_bActive = false;
}

bool RobloxObjectBrowserDoc::actionState(const QString& actionID, bool& enableState, bool& checkedState)
{
    static QString webActiveActions("fileCloseAction");
    if (webActiveActions.contains(actionID))
    {
        enableState = true;
    }
    else if (UpdateUIManager::Instance().getDockActionNames().contains(actionID))
    {
        enableState = true;
        checkedState = UpdateUIManager::Instance().getAction(actionID)->isChecked();
        return true;
    }
    else
    {
        enableState = false;
        checkedState = false;
    }

    return true;
}

void RobloxObjectBrowserDoc::onClassSelected(QListWidgetItem* current, QListWidgetItem* previous)
{
    if (!current)
        return;

    // List Widget
    ObjectBrowserItem* obItem = dynamic_cast<ObjectBrowserItem*>(current);
    m_MemberList->updateMemberList(obItem);

    // Declaration View
    m_DeclarationView->updateDeclarationView(obItem);
}

void RobloxObjectBrowserDoc::onMemberSelected(QListWidgetItem* current, QListWidgetItem* previous)
{
    if (!current)
        return;

    ObjectBrowserItem* obItem = dynamic_cast<ObjectBrowserItem*>(current);
    m_DeclarationView->updateDeclarationView(obItem);
}

void RobloxObjectBrowserDoc::initializeAllViews()
{
    // Class List
    m_ClassList->init();

    // Member List
    onClassSelected(m_ClassList->currentItem(), NULL);

    // Declaration View
}

ClassListWidget::ClassListWidget(QWidget* parent)
    : QListWidget(parent)
{
}

void ClassListWidget::init()
{
    m_ClassItemVector.clear();

    // Populate the classes
    insertClass(&Aya::Reflection::ClassDescriptor::rootDescriptor());

    std::for_each(Aya::Reflection::EnumDescriptor::enumsBegin(), Aya::Reflection::EnumDescriptor::enumsEnd(),
        boost::bind(&ClassListWidget::insertEnum, this, _1));

    // Sort and add to class list widget
    std::sort(m_ClassItemVector.begin(), m_ClassItemVector.end(), compareFunc);
    std::vector<ObjectBrowserItem*>::iterator it = m_ClassItemVector.begin();
    for (; it != m_ClassItemVector.end(); ++it)
    {
        ObjectBrowserItem* item = *it;
        addItem(item);
    }

    // Set current item in class view
    setCurrentRow(0);
}

QSize ClassListWidget::sizeHint() const
{
    QSize sz;
    sz.setHeight(QListWidget::sizeHint().height());
    sz.setWidth(75); // sizeHintForColumn(0));
    return sz;
}

void ClassListWidget::insertClass(const Aya::Reflection::ClassDescriptor* desc)
{
    if (!desc)
        return;

    Aya::Reflection::Metadata::Class* metadata = Aya::Reflection::Metadata::Reflection::singleton()->get(*desc, false);
    if (AuthoringSettings::singleton().isShown(metadata, *desc))
    {
        const Aya::Reflection::Descriptor* d = dynamic_cast<const Aya::Reflection::Descriptor*>(desc);
        if (d)
        {
            ObjectBrowserItem* item = new ObjectBrowserItem(d);
            m_ClassItemVector.push_back(item);
        }
    }

    // Recurse
    std::for_each(desc->derivedClasses_begin(), desc->derivedClasses_end(), boost::bind(&ClassListWidget::insertClass, this, _1));
}

void ClassListWidget::insertEnum(const Aya::Reflection::EnumDescriptor* enumDesc)
{
    const Aya::Reflection::Descriptor* d = dynamic_cast<const Aya::Reflection::Descriptor*>(enumDesc);
    if (d)
    {
        ObjectBrowserItem* item = new ObjectBrowserItem(d);
        item->setIcon(QIcon(QtUtilities::getPixmap(OBJECT_BROWSER_IMAGES, 9, OB_IMAGE_SIZE, true)));
        m_ClassItemVector.push_back(item);
    }
}

MemberListWidget::MemberListWidget(QWidget* parent)
    : QListWidget(parent)
    , m_descriptor(NULL)
    , m_showInheritedMembers(true)
{
}

void MemberListWidget::updateMemberList(ObjectBrowserItem* item)
{
    if (!item)
        return;

    const Aya::Reflection::Descriptor* desc = item->getDescriptor();
    if (!desc)
        return;

    // Ensure member vector is empty
    m_MemberItemVector.clear();

    // Clear the list widget for fresh view
    clear();

    m_descriptor = desc;
    if (const Aya::Reflection::ClassDescriptor* cd = dynamic_cast<const Aya::Reflection::ClassDescriptor*>(desc))
    {
        // Properties
        std::for_each(cd->begin<Aya::Reflection::PropertyDescriptor>(), cd->end<Aya::Reflection::PropertyDescriptor>(),
            boost::bind(&MemberListWidget::insertProperty, this, _1));

        // Functions
        std::for_each(cd->begin<Aya::Reflection::FunctionDescriptor>(), cd->end<Aya::Reflection::FunctionDescriptor>(),
            boost::bind(&MemberListWidget::insertFunction, this, _1));

        // Yield-Functions
        std::for_each(cd->begin<Aya::Reflection::YieldFunctionDescriptor>(), cd->end<Aya::Reflection::YieldFunctionDescriptor>(),
            boost::bind(&MemberListWidget::insertYieldFunction, this, _1));

        // Events
        std::for_each(cd->begin<Aya::Reflection::EventDescriptor>(), cd->end<Aya::Reflection::EventDescriptor>(),
            boost::bind(&MemberListWidget::insertSignal, this, _1));

        // Callbacks
        std::for_each(cd->begin<Aya::Reflection::CallbackDescriptor>(), cd->end<Aya::Reflection::CallbackDescriptor>(),
            boost::bind(&MemberListWidget::insertCallback, this, _1));
    }
    else if (const Aya::Reflection::EnumDescriptor* ed = dynamic_cast<const Aya::Reflection::EnumDescriptor*>(desc))
    {
        // Enum Items
        std::for_each(ed->begin(), ed->end(), boost::bind(&MemberListWidget::insertEnumItem, this, _1));
    }

    // Sort and add to class list widget
    std::sort(m_MemberItemVector.begin(), m_MemberItemVector.end(), compareFunc);
    std::vector<ObjectBrowserItem*>::iterator it = m_MemberItemVector.begin();
    for (; it != m_MemberItemVector.end(); ++it)
    {
        ObjectBrowserItem* item = *it;
        addItem(item);
    }
}

void MemberListWidget::insertProperty(const Aya::Reflection::PropertyDescriptor* pd)
{
    if (!pd)
        return;

    if (!m_showInheritedMembers && &pd->owner != m_descriptor)
        return;

    Aya::Reflection::Metadata::Item* metadata = Aya::Reflection::Metadata::Reflection::singleton()->get(*pd);
    if (AuthoringSettings::singleton().isHiddenInBrowser(metadata, *pd))
        return;

    // Note that we only show the "Public" properties
    // If something is LEGACY_SCRIPTABLE, it will *not* show up anymore
    if (pd->isPublic() && pd->isScriptable())
    {
        const Aya::Reflection::Descriptor* d = dynamic_cast<const Aya::Reflection::Descriptor*>(pd);
        if (d)
        {
            ObjectBrowserItem* item = new ObjectBrowserItem(d);
            const int index = SecurityToIndex(PropertyIcon, pd->security);
            item->setIcon(QIcon(QtUtilities::getPixmap(OBJECT_BROWSER_IMAGES, index, OB_IMAGE_SIZE, true)));
            m_MemberItemVector.push_back(item);
        }
    }
}

void MemberListWidget::insertFunction(const Aya::Reflection::FunctionDescriptor* fd)
{
    if (!m_showInheritedMembers && &fd->owner != m_descriptor)
        return;

    Aya::Reflection::Metadata::Item* metadata = Aya::Reflection::Metadata::Reflection::singleton()->get(*fd);
    if (AuthoringSettings::singleton().isHiddenInBrowser(metadata, *fd))
        return;

    const Aya::Reflection::Descriptor* d = dynamic_cast<const Aya::Reflection::Descriptor*>(fd);
    if (d)
    {
        ObjectBrowserItem* item = new ObjectBrowserItem(d);
        const int index = SecurityToIndex(FunctionIcon, fd->security);
        item->setIcon(QIcon(QtUtilities::getPixmap(OBJECT_BROWSER_IMAGES, index, OB_IMAGE_SIZE, true)));
        m_MemberItemVector.push_back(item);
    }
}

void MemberListWidget::insertYieldFunction(const Aya::Reflection::YieldFunctionDescriptor* yfd)
{
    if (!m_showInheritedMembers && &yfd->owner != m_descriptor)
        return;

    Aya::Reflection::Metadata::Item* metadata = Aya::Reflection::Metadata::Reflection::singleton()->get(*yfd);
    if (AuthoringSettings::singleton().isHiddenInBrowser(metadata, *yfd))
        return;

    const Aya::Reflection::Descriptor* d = dynamic_cast<const Aya::Reflection::Descriptor*>(yfd);
    if (d)
    {
        ObjectBrowserItem* item = new ObjectBrowserItem(d);
        const int index = SecurityToIndex(FunctionIcon, yfd->security);
        item->setIcon(QIcon(QtUtilities::getPixmap(OBJECT_BROWSER_IMAGES, index, OB_IMAGE_SIZE, true)));
        m_MemberItemVector.push_back(item);
    }
}

void MemberListWidget::insertSignal(const Aya::Reflection::EventDescriptor* ed)
{
    if (!ed)
        return;

    if (!ed->isPublic())
        return;

    if (!ed->isScriptable())
        return;

    if (!m_showInheritedMembers && &ed->owner != m_descriptor)
        return;

    Aya::Reflection::Metadata::Item* metadata = Aya::Reflection::Metadata::Reflection::singleton()->get(*ed);
    if (AuthoringSettings::singleton().isHiddenInBrowser(metadata, *ed))
        return;

    const Aya::Reflection::Descriptor* d = dynamic_cast<const Aya::Reflection::Descriptor*>(ed);
    if (d)
    {
        ObjectBrowserItem* item = new ObjectBrowserItem(d);
        const int index = SecurityToIndex(SignalIcon, ed->security);
        item->setIcon(QIcon(QtUtilities::getPixmap(OBJECT_BROWSER_IMAGES, index, OB_IMAGE_SIZE, true)));
        m_MemberItemVector.push_back(item);
    }
}

void MemberListWidget::insertCallback(const Aya::Reflection::CallbackDescriptor* cd)
{
    if (m_showInheritedMembers || &cd->owner == m_descriptor)
    {
        Aya::Reflection::Metadata::Item* metadata = Aya::Reflection::Metadata::Reflection::singleton()->get(*cd);
        if (AuthoringSettings::singleton().isHiddenInBrowser(metadata, *cd))
            return;

        const Aya::Reflection::Descriptor* d = dynamic_cast<const Aya::Reflection::Descriptor*>(cd);
        if (d)
        {
            ObjectBrowserItem* item = new ObjectBrowserItem(d);
            item->setIcon(QIcon(QtUtilities::getPixmap(OBJECT_BROWSER_IMAGES, 16, OB_IMAGE_SIZE, true)));
            m_MemberItemVector.push_back(item);
        }
    }
}

void MemberListWidget::insertEnumItem(const Aya::Reflection::EnumDescriptor::Item* e)
{
    if (!e)
        return;

    const Aya::Reflection::Descriptor* d = dynamic_cast<const Aya::Reflection::Descriptor*>(e);
    if (d)
    {
        ObjectBrowserItem* item = new ObjectBrowserItem(d);
        item->setIcon(QIcon(QtUtilities::getPixmap(OBJECT_BROWSER_IMAGES, 10, OB_IMAGE_SIZE, true)));
        m_MemberItemVector.push_back(item);
    }
}

DeclarationView::DeclarationView(QWidget* parent)
    : QTextBrowser(parent)
{
    setOpenLinks(true);
    setOpenExternalLinks(true);
}

static std::string getDeclaration(const Aya::Reflection::ClassDescriptor* csd)
{
    std::string result = csd->name.toString();
    while ((csd = csd->getBase()))
    {
        if (*csd == Aya::Reflection::ClassDescriptor::rootDescriptor())
            break;
        result += " -> ";
        result += csd->name.toString();
    }
    return result;
}

void DeclarationView::updateDeclarationView(ObjectBrowserItem* item)
{
    if (!item)
        return;

    const Aya::Reflection::Descriptor* desc = item->getDescriptor();
    if (!desc)
        return;

    // Clear the text edit first.
    clear();

    std::string declaration;
    std::string owner;
    bool backend = false;
    bool bRobloxRestricted = false;
    bool bDeprecated = false;
    std::string summary;

    Aya::Reflection::Metadata::Item* metadata = NULL;

    if (const Aya::Reflection::FunctionDescriptor* fd = dynamic_cast<const Aya::Reflection::FunctionDescriptor*>(desc))
    {
        const Aya::Reflection::SignatureDescriptor& signature = fd->getSignature();
        declaration = signature.resultType->name.toString();
        declaration += " ";
        declaration += fd->name.toString();
        declaration += "(";
        int count = 0;
        std::for_each(
            signature.arguments.begin(), signature.arguments.end(), boost::bind(&catenateArg<true>, boost::ref(count), boost::ref(declaration), _1));
        declaration += ")";

        if (&fd->owner)
            owner = fd->owner.name.toString();

        bRobloxRestricted = (fd->security == Aya::Security::RobloxPlace);
        metadata = Aya::Reflection::Metadata::Reflection::singleton()->get(*fd);
    }
    else if (const Aya::Reflection::YieldFunctionDescriptor* yfd = dynamic_cast<const Aya::Reflection::YieldFunctionDescriptor*>(desc))
    {
        const Aya::Reflection::SignatureDescriptor& signature = yfd->getSignature();
        declaration = signature.resultType->name.toString();
        declaration += " ";
        declaration += yfd->name.toString();
        declaration += "(";
        int count = 0;
        std::for_each(
            signature.arguments.begin(), signature.arguments.end(), boost::bind(&catenateArg<true>, boost::ref(count), boost::ref(declaration), _1));
        declaration += ")";

        if (&yfd->owner)
            owner = yfd->owner.name.toString();

        bRobloxRestricted = (yfd->security == Aya::Security::RobloxPlace);
        metadata = Aya::Reflection::Metadata::Reflection::singleton()->get(*yfd);
    }
    else if (const Aya::Reflection::PropertyDescriptor* pd = dynamic_cast<const Aya::Reflection::PropertyDescriptor*>(desc))
    {
        declaration = pd->type.name.toString();
        declaration += " ";
        declaration += pd->name.toString();
        if (&pd->owner)
            owner = pd->owner.name.toString();

        bRobloxRestricted = (pd->security == Aya::Security::RobloxPlace);
        metadata = Aya::Reflection::Metadata::Reflection::singleton()->get(*pd);
    }
    else if (const Aya::Reflection::EventDescriptor* ed = dynamic_cast<const Aya::Reflection::EventDescriptor*>(desc))
    {
        const Aya::Reflection::SignatureDescriptor& signature(ed->getSignature());
        declaration = "event ";
        declaration += ed->name.toString();
        declaration += "(";
        int count = 0;
        std::for_each(
            signature.arguments.begin(), signature.arguments.end(), boost::bind(&catenateArg<true>, boost::ref(count), boost::ref(declaration), _1));
        declaration += ")";

        if (&ed->owner)
            owner = ed->owner.name.toString();

        bRobloxRestricted = (ed->security == Aya::Security::RobloxPlace);
        metadata = Aya::Reflection::Metadata::Reflection::singleton()->get(*ed);
    }
    else if (const Aya::Reflection::CallbackDescriptor* cd = dynamic_cast<const Aya::Reflection::CallbackDescriptor*>(desc))
    {
        const Aya::Reflection::SignatureDescriptor& signature(cd->getSignature());
        declaration = "callback ";
        declaration += signature.resultType->name.c_str();
        declaration += " ";
        declaration += cd->name.toString();
        declaration += "(";
        int count = 0;
        std::for_each(
            signature.arguments.begin(), signature.arguments.end(), boost::bind(&catenateArg<true>, boost::ref(count), boost::ref(declaration), _1));
        declaration += ")";
        if (!cd->isAsync())
            declaration += " [noyield]";
        if (&cd->owner)
            owner = cd->owner.name.toString();

        metadata = Aya::Reflection::Metadata::Reflection::singleton()->get(*cd);
    }
    else if (const Aya::Reflection::EnumDescriptor::Item* edi = dynamic_cast<const Aya::Reflection::EnumDescriptor::Item*>(desc))
    {
        declaration = Aya::format("%d: %s", edi->value, edi->name.c_str());
    }
    else if (const Aya::Reflection::ClassDescriptor* csd = dynamic_cast<const Aya::Reflection::ClassDescriptor*>(desc))
    {
        declaration = getDeclaration(csd);
        metadata = Aya::Reflection::Metadata::Reflection::singleton()->get(*csd, false);
    }
    else
    {
        declaration = desc->name.toString();
    }

    if (Aya::Reflection::Metadata::Item::isDeprecated(metadata, *desc))
        bDeprecated = true;
    if (Aya::Reflection::Metadata::Item::isBackend(metadata, *desc))
        backend = true;

    if (metadata)
        summary = metadata->description;

    constructView(declaration, owner, backend, bRobloxRestricted, bDeprecated, summary);
}

void DeclarationView::constructView(std::string& decl, std::string& owner, bool backend, bool bRestricted, bool bDeprecated, std::string& summary)
{
    QString fullText;

    fullText.append("<div style=\"font-weight: bold\">");
    fullText.append(decl.c_str());
    fullText.append("</div>");
    if (!owner.empty())
    {
        fullText.append("<div>Member of <label style=\"font-weight: bold\">");
        fullText.append(owner.c_str());
        fullText.append("</label></div>");
    }

    if (backend)
    {
        fullText.append("<p style=\"color: Orange; display: none\">");
        fullText.append("<b>Back-end: </b>This item can only be used from game servers and solo games.</p>");
    }

    if (bRestricted)
    {
        fullText.append("<p style=\"color: Red; display: none\">");
        fullText.append("<b>Roblox Preliminary: </b>This item is under development and is currently restricted for use by Roblox Admins only.</p>");
    }

    if (bDeprecated)
    {
        fullText.append("<p style=\"color: Red; display: none\">");
        fullText.append("<b>Deprecated: </b>This item is deprecated. Do not use it for new work.</p>");
    }

    if (!summary.empty())
    {
        fullText.append("<p><label style=\"font-weight: bold\">");
        fullText.append("Summary:</label><br />");
        fullText.append("<label>");
        fullText.append(summary.c_str());
        fullText.append("</label></p>");
    }

    setText(fullText);
}

ObjectBrowserItem::ObjectBrowserItem(const Aya::Reflection::Descriptor* desc)
    : m_descriptor(desc)
{
    decorate(this, desc);
}
