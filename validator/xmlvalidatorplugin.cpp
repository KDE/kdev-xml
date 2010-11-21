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

#include "xmlvalidatorplugin.h"
#include "xmlschemadialog.h"
#include "xmlvalidator.h"
#include "xmlvalidatorjob.h"

#include <QtCore/QPair>
#include <QtCore/QList>
#include <QtCore/QFile>
#include <QtCore/QDir>
#include <QtGui/QApplication>

#include <KDE/KPluginFactory>
#include <KDE/KPluginLoader>
#include <KDE/KLocalizedString>
#include <KDE/KAboutData>
#include <KDE/KDebug>
#include <KDE/KMessageBox>
#include <KDE/KActionCollection>
#include <KDE/KAction>
#include <KDE/KIO/CopyJob>
#include <KDE/KTextEditor/Document>
#include <KDE/KTextEditor/View>
#include <KDE/KParts/MainWindow>

#include <interfaces/idocument.h>
#include <interfaces/idocumentcontroller.h>
#include <interfaces/iuicontroller.h>
#include <interfaces/icore.h>
#include <interfaces/iplugin.h>
#include <interfaces/isourceformatter.h>
#include <interfaces/iruncontroller.h>

#include <xmlcatalog/icatalogmanager.h>
#include <xmlcatalog/idocumentcachemanager.h>


using namespace KDevelop;

K_PLUGIN_FACTORY ( KdevPlugFactory, registerPlugin<XmlValidatorPlugin>(); )
K_EXPORT_PLUGIN ( KdevPlugFactory ( KAboutData ( "kdevxmlvalidator", "kdevxmlvalidator", ki18n ( "XML Validator" ), "0.1", ki18n ( "Validates XML files against a DTD or schema" ), KAboutData::License_GPL ) ) )


XmlValidatorPlugin::XmlValidatorPlugin ( QObject *parent, const QVariantList& )
        : IPlugin ( KdevPlugFactory::componentData(), parent ) {

    m_validateAction = actionCollection()->addAction ( "xmlvalidate_xmllanguagesupport" );
    m_validateAction->setText ( i18n ( "Validate XML" ) );
    m_validateAction->setToolTip ( i18n ( "Validate the XML" ) );
    m_validateAction->setWhatsThis ( i18n ( "<b>Validate XML</b><p> Validate the XML" ) );
    m_validateAction->setEnabled ( false );
    connect ( m_validateAction, SIGNAL ( triggered ( bool ) ), this, SLOT ( showDialog() ) );
}

XmlValidatorPlugin::~XmlValidatorPlugin() {
}

ContextMenuExtension XmlValidatorPlugin::contextMenuExtension ( Context* context ) {
    Q_UNUSED(context)

    ContextMenuExtension ext;
    m_validateAction->setEnabled ( false );
    IDocument* doc = ICore::self()->documentController()->activeDocument();
    if ( !doc )
        return ext;
    KTextEditor::Document *tdoc = doc->textDocument();
    if ( !tdoc )
        return ext;
    KMimeType::Ptr mime = doc->mimeType();
    if ( mime->is ( "application/xml" ) ||
            mime->is ( "application/xslt+xml" ) ||
            mime->is ( "application/xsd" ) ||
            mime->is ( "application/wsdl+xml" ) ||
            mime->is ( "application/x-wsdl" )) {
        m_validateAction->setEnabled ( true );
        ext.addAction ( ContextMenuExtension::EditGroup, m_validateAction );
    }

    return ext ;
}

void XmlValidatorPlugin::showDialog() {
    IDocument* doc = ICore::self()->documentController()->activeDocument();
    if ( !doc )
        return;

    KTextEditor::Document *tdoc = doc->textDocument();
    if ( !tdoc )
        return;

    QString strError;
    int line = -1;
    int col = -1;
    QDomDocument xdoc;
    if ( !xdoc.setContent ( tdoc->text(), true, &strError, &line, &col ) ) {
        KMessageBox::error ( ICore::self()->uiController()->activeMainWindow(),
                             QString ( "%1:\n%2:%3 %4" ).arg ( i18n ( "Error in document format" ) ).arg ( line ).arg ( col ).arg ( strError ) );
        return;
    }

    QHash<QString, QString> schemas;

    QDomDocumentType doctype = xdoc.doctype();
    if ( !doctype.isNull() ) {
        QString systemId = doctype.systemId();
        QString publicId = doctype.publicId();
        QString resolved;
        if (!systemId.isEmpty() || !publicId.isEmpty())
            resolved = ICatalogManager::self()->resolve(publicId, systemId);
        if (!resolved.isEmpty())
            schemas.insert(resolved, "dtd");
        if (!systemId.isEmpty() && resolved.isEmpty())
            resolved = ICatalogManager::self()->resolveUri(systemId);
        if (!resolved.isEmpty())
            schemas.insert(resolved, "dtd");
        if (!systemId.isEmpty() && resolved.isEmpty())
            schemas.insert(systemId, "dtd");
        if ( !doctype.name().isEmpty() && resolved.isEmpty())
            schemas.insert ( QString ( "DOCTYPE %1" ).arg ( doctype.name() ), "dtd" );
    }

    //Only root nodes
    QDomNodeList nodeList = xdoc.childNodes();
    for ( int i = 0; i < nodeList.size(); i++ ) {
        QDomNode node = nodeList.at ( i );

        QString resolved = ICatalogManager::self()->resolveUri(node.namespaceURI());
        if (!resolved.isEmpty())
            schemas.insert(resolved, "xsd");

        if ( node.hasAttributes() ) {
            QDomNamedNodeMap m = node.attributes();
            for ( int j = 0; j < m.count(); j++ ) {
                QDomNode attribute = m.item ( j );
                if ( attribute.localName() == "schemaLocation" ) {
                    QStringList sl = attribute.nodeValue().split ( QRegExp ( "\\s+" ) );
                    for ( int k = 0; k < sl.size() && sl.size()%2 == 0; k += 2 ) {
                        QString resolved = ICatalogManager::self()->resolveSystemId(sl[k+1]);
                        if (resolved.isEmpty())
                            resolved = ICatalogManager::self()->resolveUri(sl[k+1]);
                        if (!resolved.isEmpty())
                            schemas.insert(resolved, "xsd");
                        else
                            schemas.insert ( sl[k+1], "xsd" );
                    }
                }
                if ( attribute.localName() == "noNamespaceSchemaLocation" ) {
                    QString resolved = ICatalogManager::self()->resolveSystemId(attribute.nodeValue());
                    if (resolved.isEmpty())
                        resolved = ICatalogManager::self()->resolveUri(attribute.nodeValue());
                    if (!resolved.isEmpty())
                        schemas.insert(resolved, "xsd");
                    else
                        schemas.insert ( attribute.nodeValue(), "xsd" );
                }
            }
        }
    }

    static XmlSchemaDialog *w = ( {
        XmlSchemaDialog *w = new XmlSchemaDialog ( ICore::self()->uiController()->activeMainWindow()->widget() );
        connect ( w, SIGNAL ( okClicked() ), this, SLOT ( slotValidate() ) );
        w;
    } );
    w->setItems ( schemas );
    w->show();
}

void XmlValidatorPlugin::slotValidate() {
    QObject * o = this->sender();
    XmlSchemaDialog *w = dynamic_cast<XmlSchemaDialog *> ( o );
    if ( !w )
        return;
    w->hide();

    IDocument* doc = ICore::self()->documentController()->activeDocument();
    if ( !doc )
        return;

    QPair<QString, QString> schema = w->getItem();
    if ( !schema.first.isEmpty() ) {

        KJob *job = 0;
        if ( schema.first.startsWith ( "DOCTYPE" ) ) {
            //NOTE This is the fallback if we could not locate a dtd document,
            //or when the DTD was specified inline.
            job = XmlValidatorJob::dtdValidationJob ( doc->url().toLocalFile() );
        } else {
            //TODO A mess: if its a remote URL or if it does not have an extention we get application/octetstream
            //This was done so that the user dont have to choose; automaticaly detected?
            //The other option is to download the file if it is remote and then to parse its contents,
            //which will be slow.
            KMimeType::Ptr localMime = KMimeType::findByUrl ( KUrl ( IDocumentCacheManager::self()->getLocalUrl(schema.first)) );
            if ( localMime->is( "application/xsd" ) || localMime->is( "application/xsd+xml" )) {
                job = XmlValidatorJob::schemaValidationJob( doc->url().toLocalFile(), schema.first );
            } else if (localMime->is( "application/xml-dtd" )) {
                job = XmlValidatorJob::dtdValidationJob ( doc->url().toLocalFile(), schema.first );
            } else if (schema.second == "xsd") {
                job = XmlValidatorJob::schemaValidationJob( doc->url().toLocalFile(), schema.first );
            } else if (schema.second == "dtd") {
                job = XmlValidatorJob::dtdValidationJob ( doc->url().toLocalFile(), schema.first );
            } else {
                KMessageBox::error(ICore::self()->uiController()->activeMainWindow(),
                                   i18n("Unable to determine mime type for: ") + schema.first);
            }
        }
        if (job) {
            connect(job, SIGNAL(result(KJob *)), this, SLOT(slotValidated(KJob *)));
            ICore::self()->runController()->registerJob(job);
        }
    }
}

void XmlValidatorPlugin::slotValidated(KJob* job)
{
    //TODO idealy we want to print the messages in one of the tabs below like build.
    XmlValidatorJob * validateJob = static_cast<XmlValidatorJob *>(job);
    if (!validateJob)
        return;
    if (validateJob->error() == XmlValidator::InternalError) {
        KMessageBox::error(0, i18n("Failed: Internal Error:\n") + validateJob->errorString());
    } else if (validateJob->error() == XmlValidator::Failed) {
        KMessageBox::error(0, i18n("Failed: Error:\n") + validateJob->errorString());
    } else if (validateJob->error() == XmlValidator::Success) {
        KMessageBox::information(0, i18n("Success: The XML is valid.\n") + validateJob->errorString());
    }
}


#include "xmlvalidatorplugin.moc"
