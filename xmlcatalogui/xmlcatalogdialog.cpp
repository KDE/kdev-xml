/*****************************************************************************
 * Copyright (c) 2009 Ruan Strydom <rm3dom@gmail.com>                        *
 *                                                                           *
 * This program is free software; you can redistribute it and/or modify      *
 * it under the terms of the GNU General Public License as published by      *
 * the Free Software Foundation; either version 3 of the License, or         *
 * (at your option) any later version.                                       *
 *                                                                           *
 * This program is distributed in the hope that it will be useful,           *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of            *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the             *
 * GNU General Public License for more details.                              *
 *                                                                           *
 * You should have received a copy of the GNU General Public License         *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.     *
 *****************************************************************************/


#include "xmlcatalogdialog.h"


#include "ui_xmlcatalogform.h"
#include "xmlcatalogmodel.h"
#include "ui_addschemaform.h"

#include <QtGui/QShowEvent>

#include <KDE/KDebug>
#include <KDE/KMessageBox>
#include <KDE/KFileDialog>

#include <xmlcatalog/icatalog.h>
#include <xmlcatalog/icatalogentry.h>
#include <xmlcatalog/icatalogmanager.h>
#include <xmlcatalog/icataloggroup.h>
#include <xmlcatalog/idocumentcachemanager.h>

#include <interfaces/icore.h>
#include <interfaces/iuicontroller.h>
#include <interfaces/idocumentcontroller.h>
#include <interfaces/iruncontroller.h>


#define debug() kDebug()

class AddSchemaDialog::AddSchemaDialogPrivate : public Ui_AddSchemaForm {
public:
    AddSchemaDialogPrivate() : widget(0) {}
    virtual ~AddSchemaDialogPrivate() {}
    QWidget *widget;
};

AddSchemaDialog::AddSchemaDialog(QWidget* parent, Qt::WFlags flags): KDialog(parent, flags)
{
    d = new AddSchemaDialogPrivate;
    d->widget = new QWidget();
    d->setupUi(d->widget);
    d->uriEdt->setFilter(i18n("*.xsd *.XSD *.dtd *.DTD|DTD and XML Schema Documents\n*|All Files"));
    setMainWidget(d->widget);
    setWindowTitle(i18n("Add DTD or XML Schema"));
}

AddSchemaDialog::~AddSchemaDialog() {
    delete  d;
}

QString AddSchemaDialog::id() const
{
    return d->idEdt->text();
}

QString AddSchemaDialog::URI() const
{
    return d->uriEdt->text();
}

AddSchemaDialog::Type AddSchemaDialog::type() const
{
    if (d->idCbx->currentText().toLower().trimmed() == "system id")
        return SystemId;
    else if (d->idCbx->currentText().toLower().trimmed() == "uri")
        return Uri;
    else if (d->idCbx->currentText().toLower().trimmed() == "doctype")
        return Doctype;
    return PublicId;
}

class XmlCatalogDialog::XmlCatalogDialogPrivate : public Ui_XmlCatalogForm {
public:
    XmlCatalogDialogPrivate() : model(0), modified(false) {
    }
    virtual ~XmlCatalogDialogPrivate() {
        if (model)
            delete model;
    }
    XmlCatalogModel *model;
    KFileDialog * fileDialog;
    bool modified;
};

XmlCatalogDialog::XmlCatalogDialog ( QWidget* parent, Qt::WFlags flags ) : KDialog ( parent, flags ) {
    setWindowTitle(i18n("XML Catalog"));
    d = new XmlCatalogDialogPrivate;
    QWidget *w = new QWidget();
    d->setupUi(w);
    this->setMainWidget(w);
    d->fileDialog = new KFileDialog ( KUrl ( "~" ), i18n("*.xml *.XML *.cat *.CAT|OASIS and SGML Catalogs\n*|All Files"), this );
    connect(this, SIGNAL(okClicked()), this, SLOT(exit()));
    connect(this, SIGNAL(cancelClicked()), this, SLOT(canceled()));
    connect(d->addCatalogBtn, SIGNAL(pressed()), d->fileDialog, SLOT(show()));
    connect(d->fileDialog, SIGNAL(fileSelected(const QString&)), this, SLOT(addCatalog(const QString&)));
    connect(d->removeBtn, SIGNAL(pressed()), this, SLOT(removeItem()));
    connect(d->updateBtn, SIGNAL(pressed()), this, SLOT(updateItem()));
    connect(d->addBtn, SIGNAL(pressed()), this, SLOT(addEntry()));
    connect(d->openBtn, SIGNAL ( pressed()), this, SLOT (slotOpenDocument()));
    connect(d->catalogTree, SIGNAL ( clicked ( QModelIndex ) ), this, SLOT ( itemSelected ( QModelIndex ) ) );

    d->addBtn->setEnabled(false);
    d->removeBtn->setEnabled(false);
    d->updateBtn->setEnabled(false);
    d->openBtn->setEnabled(false);
    
    update();
}

XmlCatalogDialog::~XmlCatalogDialog() {
    delete d;
}

void XmlCatalogDialog::showEvent(QShowEvent* e)
{
    update();
    e->accept();
    QDialog::showEvent(e);
}

void XmlCatalogDialog::exit()
{
    if (d->modified) {
        d->modified = false;
        ICatalogManager::self()->save();
    }
    d->catalogTree->setModel(0);
}

bool XmlCatalogDialog::save() {
    if (!d->modified)
        return true;
    if (d->modified && KMessageBox::questionYesNo(this, i18n("Do you want to save your changes?")) == KMessageBox::Yes) {
        ICatalogManager::self()->save();
        d->modified = false;
        return true;
    }
    return false;
}

void XmlCatalogDialog::canceled() {
    d->catalogTree->setModel(0);
    if (d->modified) {
        ICatalogManager::self()->load();
    }
    d->modified = false;
}

void XmlCatalogDialog::itemSelected(const QModelIndex& index)
{
    d->addBtn->setEnabled(false);
    d->removeBtn->setEnabled(false);
    d->updateBtn->setEnabled(false);
    d->openBtn->setEnabled(false);
    if (!index.isValid())
        return;

    if (!d || !d->catalogTree)
        return;
    XmlCatalogModelNode *n = static_cast<XmlCatalogModelNode *>(index.internalPointer());
    bool readOnly = false;
    const ICatalog * catalog = 0;
    if (n->entry())
        catalog = n->entry()->catalog();
    else if (n->catalog())
        catalog = n->catalog();
    if (!catalog)
        return;
    
    if (catalog->parameter(ICatalogManager::ParameterReadonly).toBool())
        readOnly = true;


    if (readOnly) {
        d->addBtn->setEnabled(false);
        d->removeBtn->setEnabled(false);
    } else {
        if (n->catalog())
            d->addBtn->setEnabled(true);
        d->removeBtn->setEnabled(true);
    }

    //TODO need a property for system catalog so that system catalogs may not be removed.
    if (n->catalog())
        d->removeBtn->setEnabled(true);

    d->updateBtn->setEnabled(true);
    d->openBtn->setEnabled(true);
}

void XmlCatalogDialog::addCatalog(const QString& file)
{
    d->fileDialog->hide();

    if (!ICatalogManager::self()->addCatalog(KUrl(file).toLocalFile()))
        KMessageBox::error(this, i18n("Unable to add the catalog. It is possible that this catalog is not supported: %1", file));
    else
        d->modified = true;
    
    update();
}


void XmlCatalogDialog::addEntry()
{
    static AddSchemaDialog * dialog = ( {
        AddSchemaDialog *dialog = new AddSchemaDialog ( (QWidget *) KDevelop::ICore::self()->uiController()->activeMainWindow());
        connect (dialog, SIGNAL (okClicked()), this, SLOT (slotAddEntry()));
        dialog;
    });
    dialog->show();
}


void XmlCatalogDialog::removeItem()
{
    QModelIndex index = d->catalogTree->currentIndex();
    if (!index.isValid()) {
        debug() << "Invalid model index";
        KMessageBox::error(this, i18n("Unable to remove the entry."));
        return;
    }
    XmlCatalogModelNode *n = static_cast<XmlCatalogModelNode *>(index.internalPointer());
    if (!n) {
        debug() << "Null node";
        KMessageBox::error(this, i18n("Unable to remove the entry."));
        return;
    }

    QString file;
    if (n->entry() && n->entry()->catalog()) {
        ICatalogEntry *entry = n->entry();
        if (KMessageBox::questionYesNo(this, i18n("Are you sure you want remove this entry?")) == KMessageBox::Yes) {
            entry->catalog()->removeEntry(entry);
        }
    } else if (n->catalog()) {
        ICatalog *catalog = n->catalog();
        if (KMessageBox::questionYesNo(this, i18n("Are you sure you want remove this catalog and all its entries?")) == KMessageBox::Yes) {
            ICatalogManager::self()->removeCatalog(catalog->parameter(ICatalogManager::ParameterFile).toString());
        }
    }
    d->modified = true;
    update();
    return;
}

void XmlCatalogDialog::updateItem()
{
    //This so that the user can dont overide his changes.
    if (!save()) return;
    
    QStringList urllist;

    ICatalog *catalog;
    ICatalogEntry *entry;
    getSelection(catalog, entry);
    if (entry && !catalog)
        catalog = entry->catalog();
    if (!catalog)
        return;
    QString file = catalog->parameter(ICatalogManager::ParameterFile).toString();
    if (entry) {
        urllist.append(entry->URL());
    } else if (catalog) {
        foreach(entry, catalog->entries())
            urllist.append(entry->URL());
    }
    if(urllist.size() > 0) {
        KJob * job = IDocumentCacheManager::self()->createUpdateJob(urllist);
        KDevelop::ICore::self()->runController()->registerJob(job);
    }
    
    if(!catalog->parameter(ICatalogManager::ParameterReadonly).toBool()) {
        d->catalogTree->setModel(0);
        ICatalogManager::self()->removeCatalog(file);
        ICatalogManager::self()->addCatalog(file);
        update();
    }
}

void XmlCatalogDialog::slotAddEntry()
{
    QObject * sender = this->sender();
    AddSchemaDialog * dialog = dynamic_cast<AddSchemaDialog *>(sender);
    if (!dialog) {
        debug() << "Expected instance of AddSchemaDialog";
        KMessageBox::error(dialog, i18n("Unable to add the entry."));
        return;
    }
    QString id = dialog->id();
    QString uri = dialog->URI();
    bool success = false;
    
    if(id.trimmed().isEmpty() || uri.trimmed().isEmpty()) {
        KMessageBox::error(dialog, i18n("Empty entries not allowed."));
        return;
    }

    ICatalog *catalog;
    ICatalogEntry *entry;
    getSelection(catalog, entry);

    if (entry && entry->catalog())
        catalog = entry->catalog();

    if (!catalog) {
        debug() << "Null catalog";
        KMessageBox::error(dialog, i18n("Unable to add the entry."));
        return;
    }

    if (dialog->type() == AddSchemaDialog::PublicId)
        success = catalog->addPublicEntry(id, uri, QHash<QString, QVariant>());
    else if (dialog->type() == AddSchemaDialog::SystemId)
        success = catalog->addSystemEntry(id, uri, QHash<QString, QVariant>());
    else if (dialog->type() == AddSchemaDialog::Uri)
        success = catalog->addUriEntry(id, uri, QHash<QString, QVariant>());
    else
        success = catalog->addDoctypeEntry(id, uri, QHash<QString, QVariant>());
    
    if (!success)
        KMessageBox::error(dialog, i18n("Unable to add the entry. It is possible that this catalog does not support it."));
    else {
        d->modified = true;
        update();
    }
}

void XmlCatalogDialog::update()
{
    d->catalogTree->setModel(0);
    if (d->model)
        delete d->model;
    d->model = new XmlCatalogModel();
    d->catalogTree->setModel(d->model);
    //d->catalogTree->expandToDepth(1);
    d->catalogTree->resizeColumnToContents(0);
}


void XmlCatalogDialog::slotOpenDocument()
{
    //This so that the user can see his changes.
    save();
    QString file;

    ICatalog *catalog;
    ICatalogEntry *entry;
    getSelection(catalog, entry);
    if (entry) {
        if(!entry->publicId().isEmpty() && entry->catalog()) 
            file = entry->catalog()->resolvePublicId(entry->publicId());
        else if(!entry->systemId().isEmpty() && entry->catalog()) 
            file = entry->catalog()->resolveSystemId(entry->systemId());
        else if(!entry->URI().isEmpty() && entry->catalog()) 
            file = entry->catalog()->resolveUri(entry->URI());
        else if(!entry->doctype().isEmpty() && entry->catalog()) 
            file = entry->catalog()->resolveDoctype(entry->doctype());
        else 
            file = entry->URL();
    }
    if (catalog)
        file = catalog->parameter(ICatalogManager::ParameterFile).toString();

    QString localFile = IDocumentCacheManager::self()->cachedUrl(file);
    if (QFile::exists(localFile))
        KDevelop::ICore::self()->documentController()->openDocument(KUrl(localFile));
}


void XmlCatalogDialog::getSelection(ICatalog *&catalog, ICatalogEntry *&entry)
{
    entry = 0;
    catalog = 0;
    QModelIndex index = d->catalogTree->currentIndex();
    if (!index.isValid()) {
        debug() << "Invalid model index";
        KMessageBox::error(this, i18n("Unable to add the entry."));
        return;
    }
    XmlCatalogModelNode *n = static_cast<XmlCatalogModelNode *>(index.internalPointer());
    if (!n) {
        debug() << "Null node";
        KMessageBox::error(this, i18n("Unable to add the entry."));
        return;
    }
    entry = n->entry();
    catalog = n->catalog();
}


#include "xmlcatalogdialog.moc"





