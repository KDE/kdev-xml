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

#include "xmldocumentplugin.h"
#include "xmldocument.h"

#include <KDE/KPluginFactory>
#include <KDE/KPluginLoader>
#include <KDE/KLocalizedString>
#include <KDE/KAboutData>
#include <KDE/KDebug>
#include <KDE/KMessageBox>
#include <KDE/KActionCollection>
#include <KDE/KAction>
#include <KDE/KTextEditor/Document>

#include <interfaces/icore.h>
#include <interfaces/contextmenuextension.h>
#include <interfaces/context.h>
#include <interfaces/idocument.h>
#include <interfaces/iuicontroller.h>
#include <interfaces/idocumentcontroller.h>
#include <interfaces/isourceformattercontroller.h>
#include <interfaces/isourceformatter.h>

#include <language/duchain/declaration.h>
#include <language/duchain/duchainutils.h>
#include <language/duchain/duchain.h>
#include <language/duchain/duchainlock.h>
#include <language/duchain/classdeclaration.h>
#include <language/duchain/classmemberdeclaration.h>

#include <interfaces/isourceformattercontroller.h>

using namespace KDevelop;


K_PLUGIN_FACTORY ( KdevPlugFactory, registerPlugin<XmlDocumentPlugin>(); )
K_EXPORT_PLUGIN ( KdevPlugFactory ( KAboutData ( "kdevxmldocument", "kdevxmldocument", ki18n ( "XML Document Tree" ), "0.1", ki18n ( "Displays the XML structure as a tree" ), KAboutData::License_GPL ) ) )

XmlDocumentPlugin::XmlDocumentPlugin ( QObject* parent, const QVariantList& )
        : IPlugin ( KdevPlugFactory::componentData(), parent ), m_docFactory ( 0 ) {
    m_docFactory = new XMLDocumentFactory();
    if ( m_docFactory ) {
        KDevelop::IDocumentController* idc = ICore::self()->documentController();
        idc->registerDocumentForMimetype ( "application/xml", m_docFactory );
        idc->registerDocumentForMimetype ( "application/xsd", m_docFactory );
        idc->registerDocumentForMimetype ( "application/wsdl+xml", m_docFactory );
        idc->registerDocumentForMimetype ( "application/x-wsdl", m_docFactory );
        idc->registerDocumentForMimetype ( "application/xslt+xml", m_docFactory );
    }
}

XmlDocumentPlugin::~XmlDocumentPlugin() {

}


