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

#include "formatterplugin.h"
#include "formatter.h"
#include "sgmlformatter.h"
#include "xmlformatter.h"
#include "htmlformatter.h"
#include "formatterpreferences.h"

#include <QtCore/QPair>
#include <QtCore/QList>

#include <KDE/KPluginFactory>
#include <KDE/KPluginLoader>
#include <KDE/KLocalizedString>
#include <KDE/KAboutData>
#include <KDE/KTextEditor/Document>
#include <KDE/KDebug>
#include <KDE/KMessageBox>
#include <KDE/KActionCollection>
#include <KDE/KAction>
#include <KDE/KTextEditor/View>

#include <interfaces/icore.h>
#include <interfaces/iplugin.h>
#include <interfaces/isourceformatter.h>
#include <interfaces/idocument.h>
#include <interfaces/idocumentcontroller.h>
#include "htmlformatter.h"
#include <interfaces/isourceformattercontroller.h>


using namespace KDevelop;
using namespace Xml;

K_PLUGIN_FACTORY ( KdevPlugFactory, registerPlugin<Xml::FormatterPlugin>(); )
K_EXPORT_PLUGIN ( KdevPlugFactory ( KAboutData ( "kdevsgmlformatter", "kdevsgmlformatter", ki18n ( "SGML Formatter" ), "0.1", ki18n ( "A plugin for formatting SGML/XML files" ), KAboutData::License_GPL ) ) )


FormatterPlugin::FormatterPlugin ( QObject *parent, const QVariantList& )
        : IPlugin ( KdevPlugFactory::componentData(), parent ) {
    KDEV_USE_EXTENSION_INTERFACE ( ISourceFormatter );

    SourceFormatterStyle defaultXmlStyle("XML");
    defaultXmlStyle.setContent("INDENT=4");
    defaultXmlStyle.setCaption("XML");
    m_style = defaultXmlStyle;
    m_styles.append(m_style);

    SourceFormatterStyle defaultHtmlStyle("HTML");
    defaultHtmlStyle.setContent("INDENT=4,CASE=LOWER,SOURCES=TRUE");
    defaultHtmlStyle.setCaption("HTML");
    m_styles.append(defaultHtmlStyle);

    SourceFormatterStyle defaultSgmlStyle("SGML");
    defaultSgmlStyle.setContent("INDENT=4");
    defaultSgmlStyle.setCaption("SGML");
    m_styles.append(defaultSgmlStyle);

    m_compactAction = actionCollection()->addAction ( "compact_sgmllanguagesupport" );
    m_compactAction->setText ( i18n ( "Compact source" ) );
    m_compactAction->setToolTip ( i18n ( "Compact the source by removing all white-space characters" ) );
    m_compactAction->setWhatsThis ( i18n ( "<b>Compact source</b><p> Compact the source by removing all white-space characters</p>" ) );
    m_compactAction->setEnabled ( false );
    connect ( m_compactAction, SIGNAL ( triggered ( bool ) ), this, SLOT ( slotCompactXml() ) );

    m_escapeAction = actionCollection()->addAction ( "escape_sgmllanguagesupport" );
    m_escapeAction->setText ( i18n ( "Escape source" ) );
    m_escapeAction->setToolTip ( i18n ( "Encode the source by escaping all unsafe characters" ) );
    m_escapeAction->setWhatsThis ( i18n ( "<b>Encode source</b><p> Encode the source by escaping all unsafe characters</p>" ) );
    m_escapeAction->setEnabled ( false );
    connect ( m_escapeAction, SIGNAL ( triggered ( bool ) ), this, SLOT ( slotEncodeXml() ) );

    m_unescapeAction = actionCollection()->addAction ( "unescape_sgmllanguagesupport" );
    m_unescapeAction->setText ( i18n ( "Un-escape source" ) );
    m_unescapeAction->setToolTip ( i18n ( "Un-escape the source by removing all escaped strings" ) );
    m_unescapeAction->setWhatsThis ( i18n ( "<b>Un-escape source</b><p> Un-escape the source by removing all escaped strings</p>" ) );
    m_unescapeAction->setEnabled ( false );
    connect ( m_unescapeAction, SIGNAL ( triggered ( bool ) ), this, SLOT ( slotDecodeXml() ) );
}

FormatterPlugin::~FormatterPlugin() {
}

ContextMenuExtension FormatterPlugin::contextMenuExtension ( Context* context ) {
    Q_UNUSED(context);

    ContextMenuExtension ext;

    m_compactAction->setEnabled ( false );

    IDocument* doc = ICore::self()->documentController()->activeDocument();
    if ( !doc )
        return ext;

    KTextEditor::Document *tdoc = doc->textDocument();
    if ( !tdoc )
        return ext;

    KMimeType::Ptr mime = doc->mimeType();
    if ( mime->is("application/xml") ||
            mime->is("application/xsd") ||
            mime->is("application/xslt+xml") ||
            mime->is("application/wsdl+xml") ||
            mime->is("application/x-wsdl") ||
            mime->is("text/html") ||
            mime->is("application/docbook+xml")) {
        m_compactAction->setEnabled ( true );
        m_unescapeAction->setEnabled ( true );
        m_escapeAction->setEnabled ( true );
        ext.addAction ( ContextMenuExtension::EditGroup, m_compactAction );
        ext.addAction ( ContextMenuExtension::EditGroup, m_unescapeAction );
        ext.addAction ( ContextMenuExtension::EditGroup, m_escapeAction );
    }

    return ext ;
}

QString FormatterPlugin::name() {
    return "sgmlformatter";
}

QString FormatterPlugin::caption() {
    return "SGML Formatter";
}

QString FormatterPlugin::description() {
    return i18n ( "<b>SGML Formatter</b> is a SGML/XML formatter." );
}

QString FormatterPlugin::highlightModeForMime ( const KMimeType::Ptr &mime ) {
    if(mime) {
        if ( mime->is("application/xml") ||
                mime->is("application/xsd") ||
                mime->is("application/xslt+xml") ||
                mime->is("application/wsdl+xml") ||
                mime->is("application/x-wsdl")) {
            return "XML";
        }
        if ( mime->is("text/html")) {
            return "HTML";
        }
    }
    return "SGML";
}

QString FormatterPlugin::formatSource(const QString& text, const KUrl& url, const KMimeType::Ptr& mime, const QString& leftContext = QString(), const QString& rightContext = QString())
{
    m_style = ICore::self()->sourceFormatterController()->styleForMimeType(mime);
    return formatSourceWithStyle(m_style, text, url, mime, leftContext, rightContext);
}

QString FormatterPlugin::formatSourceWithStyle(SourceFormatterStyle style, const QString& text, const KUrl& url, const KMimeType::Ptr& mime, const QString& leftContext = QString(), const QString& rightContext = QString())
{
    Formatter * formatter = formatterForMime(mime);
    formatter->loadStyle(style.content());
    formatter->setMime(mime);
    return formatter->formatSource ( text, leftContext, rightContext );
}

QList< SourceFormatterStyle > FormatterPlugin::predefinedStyles()
{
    return m_styles;
}

void FormatterPlugin::setStyle(const KDevelop::SourceFormatterStyle& style)
{
    m_style = style;
}

SourceFormatterStyle FormatterPlugin::style() const
{
    return m_style;
}

KDevelop::SettingsWidget* FormatterPlugin::editStyleWidget ( const KMimeType::Ptr &mime ) {
    Q_UNUSED ( mime );
    return new FormatterPreferences ( 0 );
}

QString FormatterPlugin::previewText ( const KMimeType::Ptr & ) {
    return QString ( "<?xml version='1.0' encoding='UTF-8'?>\n" ) +
           "<Quote type=\"virtual\" >\n" +
           "  <ContactAboutQuote value=\"true\" />\n" +
           "  <SendMarketingMaterial value=\"false\" />\n" +
           "  <DisplayCommission value=\"false\" />\n" +
           "  <RequestingEntity entityId=\"1051920013\" >\n" +
           "    <RequestingEntityAttribute attribute=\"Name\" >Peterpan</RequestingEntityAttribute>\n" +
           "  </RequestingEntity>\n" +
           "</Quote>\n";
}

ISourceFormatter::IndentationType FormatterPlugin::indentationType() {
    return ISourceFormatter::IndentWithSpacesAndConvertTabs;
}

int FormatterPlugin::indentationLength() {
    Formatter * formatter = formatterForMime(KMimeType::Ptr());
    formatter->loadStyle(m_style.content());
    return formatter->options() ["INDENT"].toInt();
}

void FormatterPlugin::slotCompactXml() {
    IDocument* doc = ICore::self()->documentController()->activeDocument();
    if ( !doc )
        return;

    KTextEditor::Document *tdoc = doc->textDocument();
    if ( !tdoc )
        return;
    
    Formatter * formatter = formatterForMime(doc->mimeType());
    formatter->loadStyle(m_style.content());

    if ( tdoc->activeView()->selection() )
        tdoc->replaceText ( tdoc->activeView()->selectionRange(),
                            formatter->compactSource ( tdoc->activeView()->selectionText() ) );
    else
        tdoc->setText ( formatter->compactSource ( tdoc->text() ) );
}

void FormatterPlugin::slotDecodeXml() {
    IDocument* doc = ICore::self()->documentController()->activeDocument();
    if ( !doc )
        return;

    KTextEditor::Document *tdoc = doc->textDocument();
    if ( !tdoc )
        return;

    Formatter * formatter = formatterForMime(doc->mimeType());
    formatter->loadStyle(m_style.content());
    
    if ( tdoc->activeView()->selection() )
        tdoc->replaceText ( tdoc->activeView()->selectionRange(),
                            formatter->unescapeSource ( tdoc->activeView()->selectionText() ) );
    else
        tdoc->setText ( formatter->unescapeSource ( tdoc->text() ) );

}

void FormatterPlugin::slotEncodeXml() {
    IDocument* doc = ICore::self()->documentController()->activeDocument();
    if ( !doc )
        return;

    KTextEditor::Document *tdoc = doc->textDocument();
    if ( !tdoc )
        return;

    Formatter * formatter = formatterForMime(doc->mimeType());
    formatter->loadStyle(m_style.content());

    if ( tdoc->activeView()->selection() )
        tdoc->replaceText ( tdoc->activeView()->selectionRange(),
                            formatter->escapeSource ( tdoc->activeView()->selectionText() ) );
    else
        tdoc->setText ( formatter->escapeSource ( tdoc->text() ) );
}

Formatter* FormatterPlugin::formatterForMime(KMimeType::Ptr mime)
{
    static Formatter *xmlFormatter = new XmlFormatter();
    static Formatter *sgmlFormatter = new SgmlFormatter();
    static Formatter *htmlFormatter = new HtmlFormatter();

    if(!mime) {
        return sgmlFormatter;
    }

    if ( mime->is("text/xml") ||
            mime->is("application/xml") ||
            mime->is("application/xsd") ||
            mime->is("application/xslt+xml") ||
            mime->is("application/wsdl+xml") ||
            mime->is("application/x-wsdl")) {
        return xmlFormatter;
    }

    if (mime->is("text/html")) {
        return htmlFormatter;
    }

    return sgmlFormatter;
}


#include "formatterplugin.moc"


