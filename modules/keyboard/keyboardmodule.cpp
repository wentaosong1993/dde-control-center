#include "keyboardmodule.h"
#include "keyitem.h"
#include "keyboardmodel.h"
#include "customedit.h"

namespace dcc {
namespace keyboard{

KeyboardModule::KeyboardModule(FrameProxyInterface *frame, QObject *parent)
    :QObject(parent),
      ModuleInterface(frame),
      m_loaded(false),
      m_keyboardWidget(nullptr),
      m_kbDetails(nullptr),
      m_kbLayoutWidget(nullptr),
      m_shortcutWidget(nullptr),
      m_langWidget(nullptr),
      m_scContent(nullptr),
      m_customContent(nullptr)
{
}

void KeyboardModule::initialize()
{
    m_model = new KeyboardModel();
    m_shortcutModel = new ShortcutModel();
    m_work = new KeyboardWork(m_model);

    connect(m_work, SIGNAL(shortcutInfo(QString)), m_shortcutModel, SLOT(onParseInfo(QString)));
    connect(m_work, SIGNAL(customInfo(QString)), m_shortcutModel, SLOT(onCustomInfo(QString)));

    m_model->moveToThread(qApp->thread());
    m_shortcutModel->moveToThread(qApp->thread());
    m_work->moveToThread(qApp->thread());
}

void KeyboardModule::moduleActive()
{
    m_work->active();
}

void KeyboardModule::moduleDeactive()
{
    m_work->deactive();
}

void KeyboardModule::reset()
{

}

ModuleWidget *KeyboardModule::moduleWidget()
{
    if(!m_keyboardWidget)
    {
        m_keyboardWidget = new KeyboardWidget(m_model);

        connect(m_keyboardWidget, SIGNAL(keyoard()), this, SLOT(onPushKBDetails()));
        connect(m_keyboardWidget, SIGNAL(language()), this, SLOT(onPushLanguage()));
        connect(m_keyboardWidget, SIGNAL(shortcut()), this, SLOT(onPushShortcut()));
        connect(m_keyboardWidget, &KeyboardWidget::delayChanged, m_work, &KeyboardWork::setRepeatDelay);
        connect(m_keyboardWidget, &KeyboardWidget::speedChanged, m_work, &KeyboardWork::setRepeatInterval);
        connect(m_keyboardWidget, &KeyboardWidget::numLockChanged, m_work, &KeyboardWork::setNumLock);
        connect(m_keyboardWidget, &KeyboardWidget::capsLockChanged, m_work, &KeyboardWork::setCapsLock);
    }

    return m_keyboardWidget;
}

const QString KeyboardModule::name() const
{
    return QStringLiteral("keyboard");
}

void KeyboardModule::contentPopped(ContentWidget * const w)
{
    if(w == m_kbDetails)
        m_kbDetails = nullptr;
    else if(w == m_kbLayoutWidget)
        m_kbLayoutWidget = nullptr;
    else if(w == m_shortcutWidget)
        m_shortcutWidget = nullptr;
    else if(w == m_langWidget)
        m_langWidget = nullptr;
    else if(w == m_scContent)
        m_scContent = nullptr;
    else if(w == m_customContent)
        m_customContent = nullptr;

    w->deleteLater();
}

ShortcutInfo *KeyboardModule::checkConflict(const QString &shortcut, QStringList &list)
{
    QString dest = shortcut;
    dest = dest.replace("Control", "Ctrl");
    dest = dest.replace("<", "");
    dest = dest.replace(">", "-");
    QStringList dests = dest.split("-");
    QList<ShortcutInfo*> infos = m_shortcutModel->infos();

    QList<ShortcutInfo*>::iterator itinfo = infos.begin();
    ShortcutInfo* conflict = NULL;
    QStringList checkeds;

    int dests_count = 0;
    for (QString &t : dests){
        if (t == "Ctrl")
            dests_count += m_work->Modifier::control;
        else if (t == "Alt")
            dests_count += m_work->Modifier::alt;
        else if (t == "Super")
            dests_count += m_work->Modifier::super;
        else if (t == "Shift")
            dests_count += m_work->Modifier::shift;
        else {
            QString s = t;
            s = ModelKeycode.value(s);
            if (!s.isEmpty())
                t = s;
        }
    }

    for(; itinfo != infos.end(); ++itinfo)
    {
        ShortcutInfo* item = (*itinfo);
        QString src = item->accels;
        src = src.replace("Control", "Ctrl");
        src = src.replace("<", "");
        src = src.replace(">", "-");
        QStringList srcs = src.split("-");

        int srcs_count = 0;
        for (QString &t : srcs){
            if (t == "Ctrl")
                srcs_count += m_work->Modifier::control;
            else if (t == "Alt")
                srcs_count += m_work->Modifier::alt;
            else if (t == "Super")
                srcs_count += m_work->Modifier::super;
            else if (t == "Shift")
                srcs_count += m_work->Modifier::shift;
            else {
                QString s = t;
                s = ModelKeycode.value(s);
                if (!s.isEmpty())
                    t = s;
            }
        }

        if (srcs_count == dests_count && converKey(dests.last()) == converKey(srcs.last()))
            conflict = item;
    }

    QList<KeyItem*> items = KeyItem::keyboards();
    QList<KeyItem*>::iterator it = items.begin();

    for(; it != items.end(); ++it)
    {
        if(conflict)
        {
            if(checkeds.contains((*it)->mainKey()) || dests.contains((*it)->mainKey()))
            {
                (*it)->setPress(true);
                (*it)->setConflict(true);
            }
            else
            {
                (*it)->setPress(false);
                (*it)->setConflict(false);
            }
        }
        else
        {
            (*it)->setPress(false);
            (*it)->setConflict(false);
        }
    }
    checkeds<<dests;
    list = checkeds;
    return conflict;
}

QString KeyboardModule::converKey(const QString &key)
{
     QString converkey = ModelKeycode.value(key);
     if (converkey.isEmpty())
         return key;
     else
         return converkey;
}

void KeyboardModule::onPushKeyboard()
{
    if(!m_kbLayoutWidget)
    {
        m_kbLayoutWidget = new KeyboardLayoutWidget();
        m_kbLayoutWidget->setMetaData(m_work->getDatas());
        m_kbLayoutWidget->setLetters(m_work->getLetters());

        connect(m_kbLayoutWidget, SIGNAL(layoutSelected()), this, SLOT(onKeyboardLayoutSelected()));
    }
    m_frameProxy->pushWidget(this, m_kbLayoutWidget);
}

void KeyboardModule::onPushKBDetails()
{
    if(!m_kbDetails)
    {
        m_kbDetails = new KeyboardDetails();
        QStringList userlayout = m_model->userLayout();
        QString cur = m_model->curLayout();
        for(int i = 0; i<userlayout.count(); i++)
        {
            MetaData md;
            QString key = userlayout.at(i);
            QString text = m_model->layoutByValue(key);

            md.setKey(key);
            md.setText(text);
            md.setSelected(text == cur);
            m_kbDetails->onAddKeyboard(md);
        }
        connect(m_kbDetails, SIGNAL(layoutAdded()), this, SLOT(onPushKeyboard()));
        connect(m_kbDetails, SIGNAL(curLayout(QString)), this, SLOT(setCurrentLayout(QString)));
        connect(m_kbDetails, SIGNAL(delUserLayout(QString)), m_work, SLOT(delUserLayout(QString)));
    }

    m_frameProxy->pushWidget(this, m_kbDetails);
}

void KeyboardModule::onPushLanguage()
{
    if(!m_langWidget)
    {
        m_langWidget = new LangWidget(m_model);
        connect(m_langWidget, SIGNAL(click(QModelIndex)), this, SLOT(onSetLocale(QModelIndex)));
    }

    m_frameProxy->pushWidget(this, m_langWidget);
}

void KeyboardModule::onPushShortcut()
{
    if(!m_shortcutWidget)
    {
        m_shortcutWidget = new ShortcutWidget(m_shortcutModel);
        connect(m_shortcutWidget, SIGNAL(shortcutChanged(bool, ShortcutInfo* , QString)), this, SLOT(onShortcutChecked(bool, ShortcutInfo*, QString)));
        connect(m_shortcutWidget, SIGNAL(customShortcut()), this, SLOT(onPushCustomShortcut()));
        connect(m_shortcutWidget, SIGNAL(delShortcutInfo(ShortcutInfo*)), this, SLOT(onDelShortcut(ShortcutInfo*)));
        connect(m_work, SIGNAL(searchChangd(ShortcutInfo*,QString)), m_shortcutWidget, SLOT(onSearchInfo(ShortcutInfo*,QString)));
        connect(m_shortcutWidget, &ShortcutWidget::requestDisableShortcut, m_work, &KeyboardWork::onDisableShortcut);
        connect(m_shortcutWidget, &ShortcutWidget::shortcutEditChanged, this, &KeyboardModule::onShortcutEdit);

    }
    m_frameProxy->pushWidget(this, m_shortcutWidget);
}

void KeyboardModule::onPushCustomShortcut()
{
    if(!m_customContent)
    {
        m_customContent = new CustomContent(m_work);
        connect(m_customContent, SIGNAL(shortcut(QString)), this, SLOT(onShortcutSet(QString)));
    }

    m_frameProxy->pushWidget(this, m_customContent);
}

void KeyboardModule::onKeyboardLayoutSelected()
{
    if(m_kbLayoutWidget)
    {
        QList<MetaData> datas = m_kbLayoutWidget->selectData();
        for(int i = 0; i<datas.count(); i++)
        {
            if(m_kbDetails)
            {
                MetaData data = datas.at(i);
                m_kbDetails->onAddKeyboard(data);
                m_work->addUserLayout(data.key());
            }
        }
    }
}

void KeyboardModule::setCurrentLayout(const QString& value)
{
    m_work->setLayout(value);
}

void KeyboardModule::setCurrentLang()
{

}

void KeyboardModule::onSetLocale(const QModelIndex &index)
{
    QVariant var = index.data();
    MetaData md = var.value<MetaData>();

    m_work->setLang(md.key());
}

void KeyboardModule::onShortcutChecked(bool valid, ShortcutInfo* info, const QString &shortcut)
{
    if(valid)
    {
        m_work->modifyShortcut(info, shortcut);
    }
    else
    {
        QString dest = shortcut;
        dest.replace("Control","Ctrl");
        dest = dest.replace("<", "");
        dest = dest.replace(">", "-");
        QStringList dests = dest.split("-");
        QList<ShortcutInfo*> infos = m_shortcutModel->infos();


        int dests_count = 0;
        for (QString &t : dests){
            if (t == "Ctrl")
                dests_count += m_work->Modifier::control;
            else if (t == "Alt")
                dests_count += m_work->Modifier::alt;
            else if (t == "Super")
                dests_count += m_work->Modifier::super;
            else if (t == "Shift")
                dests_count += m_work->Modifier::shift;
            else {
                QString s = t;
                s = ModelKeycode.value(s);
                if (!s.isEmpty())
                    t = s;
            }
        }

        QList<ShortcutInfo*>::iterator itinfo = infos.begin();
        ShortcutInfo* conflict = NULL;
        QStringList checkeds;
        for(; itinfo != infos.end(); ++itinfo)
        {
            ShortcutInfo* item = (*itinfo);
            QString src = item->accels;
            src = src.replace("Control", "Ctrl");
            src = src.replace("<", "");
            src = src.replace(">", "-");
            QStringList srcs = src.split("-");

            int srcs_count = 0;
            for (QString &t : srcs){
                if (t == "Ctrl")
                    srcs_count += m_work->Modifier::control;
                else if (t == "Alt")
                    srcs_count += m_work->Modifier::alt;
                else if (t == "Super")
                    srcs_count += m_work->Modifier::super;
                else if (t == "Shift")
                    srcs_count += m_work->Modifier::shift;
                else {
                    QString s = t;
                    s = ModelKeycode.value(s);
                    if (!s.isEmpty())
                        t = s;
                }
            }
            if (srcs_count == dests_count && converKey(dests.last()) == converKey(srcs.last()))
                conflict = item;
        }

        QList<KeyItem*> items = KeyItem::keyboards();
        QList<KeyItem*>::iterator it = items.begin();
        for(; it != items.end(); ++it)
        {
//            if(checkeds.contains(dests.last()))
            {
                if(checkeds.contains((*it)->mainKey()) || dests.contains((*it)->mainKey()))
                {
                    (*it)->setPress(true);
                }
                else
                    (*it)->setPress(false);
            }
        }

        if(!m_scContent)
        {
            m_scContent = new ShortcutContent(m_work);
            connect(m_scContent, &ShortcutContent::shortcut, this, &KeyboardModule::onShortcutKeySet);
            checkeds<<dests;
            m_scContent->setConflictString(checkeds);
            if(conflict)
            {
                m_scContent->setBottomTip(conflict);
                m_scContent->setCurInfo(info);
            }
        }
        m_frameProxy->pushWidget(this, m_scContent);
    }
}

void KeyboardModule::onShortcutSet(const QString &shortcut)
{
    QStringList list;
    ShortcutInfo* conflict = checkConflict(shortcut, list);

    if(m_customContent)
    {
        m_customContent->setBottomTip(conflict);
        m_customContent->setConflictString(list);
    }
}

void KeyboardModule::onShortcutKeySet(const QString &shortcut)
{
    QStringList list;
    ShortcutInfo* conflict = checkConflict(shortcut, list);

    if(m_scContent)
    {
        m_scContent->setBottomTip(conflict);
        m_scContent->setConflictString(list);
    }
}

void KeyboardModule::onDelShortcut(ShortcutInfo *info)
{
    m_work->delShortcut(info);
    m_shortcutModel->delInfo(info);
}

void KeyboardModule::onDelay(int value)
{
    m_work->setRepeatDelay(value);
}

void KeyboardModule::onSpeed(int value)
{
    m_work->setRepeatInterval(value);
}

void KeyboardModule::onShortcutEdit(ShortcutInfo *info)
{
    CustomEdit *w = new CustomEdit(m_work);
    w->setShortcut(info);

    ShortcutWidget *shortcutWidget = static_cast<ShortcutWidget*>(sender());
    SettingsHead *head = shortcutWidget->getHead();

    connect(w, &CustomEdit::requestEditFinished, head, &SettingsHead::toCancel);
    connect(w, &CustomEdit::shortcutChangd, this, &KeyboardModule::onShortcutChecked);
    connect(w, &CustomEdit::requestDisableShortcut, m_work, &KeyboardWork::onDisableShortcut);

    m_frameProxy->pushWidget(this, w);
}

void KeyboardModule::setCapsLock(bool value)
{
    m_work->setCapsLock(value);
}

KeyboardModule::~KeyboardModule()
{
    m_work->deleteLater();
    m_model->deleteLater();
    m_shortcutModel->deleteLater();
}

}
}
