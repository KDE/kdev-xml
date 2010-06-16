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

#include "catalogmanager.h"
#include "oasiscatalog.h"
#include "oasiscatalogreaderwriter.h"
#include "catalog_debug.h"

#include <QtCore/QFile>

#include <KDE/KMimeType>
#include <KDE/KFile>
#include <KDE/KDebug>
#include <KDE/KLocalizedString>
#include <KDE/KStandardDirs>

#include "sgmlcatalogreaderwriter.h"

CatalogManager::CatalogManager() {
    m_rw.append(new OASISCatalogReaderWriter);
    m_rw.append(new SGMLCatalogReaderWriter);
    load();
}

CatalogManager::~CatalogManager() {
    for ( int i = 0; i < m_ctlg.size(); i++ )
        delete m_ctlg[i];
    for ( int i = 0; i < m_sysctlg.size(); i++ )
        delete m_sysctlg[i];
    for ( int i = 0; i < m_rw.size(); i++ )
        delete m_rw[i];
}

QList< ICatalog* > CatalogManager::catalogs() const
{
    QList< ICatalog* > ctlgs;
    ctlgs.append(m_ctlg);
    ctlgs.append(m_sysctlg);
    return ctlgs;
}

const ICatalogReaderWriter* CatalogManager::writer(const ICatalog* catalog) const
{
    if (!catalog)
        return 0;
    for ( int i = 0; i < m_rw.size(); i++ ) {
        if ( m_rw[i] && m_rw[i]->accepts(catalog) )
            return m_rw[i];
    }
    return 0;
}

const ICatalogReaderWriter* CatalogManager::readerForContent ( const QString& file ) const {
    for ( int i = 0; i < m_rw.size(); i++ ) {
        if ( m_rw[i] && m_rw[i]->accepts ( file ) )
            return m_rw[i];
    }
    return 0;
}


QString CatalogManager::resolve ( const QString& publicId, const QString& systemId ) const {
    QString r;
    for ( int i = 0; i < m_ctlg.size(); i++ ) {
        if ( m_ctlg[i] )
            r = m_ctlg[i]->resolve ( publicId, systemId );
        if ( !r.isEmpty() )
            return r;
    }
    for ( int i = 0; i < m_sysctlg.size(); i++ ) {
        if ( m_sysctlg[i] )
            r = m_sysctlg[i]->resolve ( publicId, systemId );
        if ( !r.isEmpty() )
            return r;
    }
    return QString::null;
}

QString CatalogManager::resolvePublicId ( const QString& publicId ) const {
    if (publicId.isEmpty())
        return QString::null;
    QString r;
    for ( int i = 0; i < m_ctlg.size(); i++ ) {
        if ( m_ctlg[i] )
            r = m_ctlg[i]->resolvePublicId ( publicId );
        if ( !r.isEmpty() )
            return r;
    }
    for ( int i = 0; i < m_sysctlg.size(); i++ ) {
        if ( m_sysctlg[i] )
            r = m_sysctlg[i]->resolvePublicId ( publicId );
        if ( !r.isEmpty() )
            return r;
    }
    return QString::null;
}

QString CatalogManager::resolveSystemId ( const QString& systemId ) const {
    if (systemId.isEmpty())
        return QString::null;
    QString r;
    for ( int i = 0; i < m_ctlg.size(); i++ ) {
        if ( m_ctlg[i] )
            r = m_ctlg[i]->resolveSystemId ( systemId );
        if ( !r.isEmpty() )
            return r;
    }
    for ( int i = 0; i < m_sysctlg.size(); i++ ) {
        if ( m_sysctlg[i] )
            r = m_sysctlg[i]->resolveSystemId ( systemId );
        if ( !r.isEmpty() )
            return r;
    }
    return QString::null;
}

QString CatalogManager::resolveUri(const QString& uri) const
{
    if (uri.isEmpty())
        return QString::null;
    QString r;
    for ( int i = 0; i < m_ctlg.size(); i++ ) {
        if ( m_ctlg[i] )
            r = m_ctlg[i]->resolveUri ( uri );
        if ( !r.isEmpty() )
            return r;
    }
    for ( int i = 0; i < m_sysctlg.size(); i++ ) {
        if ( m_sysctlg[i] )
            r = m_sysctlg[i]->resolveUri ( uri );
        if ( !r.isEmpty() )
            return r;
    }
    return QString::null;
}

QString CatalogManager::resolveDoctype(const QString& doctype) const
{
    if (doctype.isEmpty())
        return QString::null;
    QString r;
    for ( int i = 0; i < m_ctlg.size(); i++ ) {
        if ( m_ctlg[i] )
            r = m_ctlg[i]->resolveDoctype ( doctype );
        if ( !r.isEmpty() )
            return r;
    }
    for ( int i = 0; i < m_sysctlg.size(); i++ ) {
        if ( m_sysctlg[i] )
            r = m_sysctlg[i]->resolveDoctype ( doctype );
        if ( !r.isEmpty() )
            return r;
    }
    return QString::null;
}


bool CatalogManager::addCatalog ( const QString& file, const QString &name ) {
    return addCatalog(m_ctlg, file, name, false);
}

bool CatalogManager::addCatalog(QList<ICatalog *> &ctlg, const QString& file, const QString& name, bool readOnly)
{
    debug() << "Loading catalog:" << file;
  
    if (file.isEmpty())
        return false;

    QFile f ( file );

    if (!f.exists()) {
        debug() << "Catalog does not exist:" << file;
        return false;
    }

    foreach(ICatalog *c, ctlg) {
        if (c->parameter(ParameterFile) == file)
            return true;
    }

    const ICatalogReaderWriter * rw = readerForContent ( file );
    if ( !rw ) {
        debug() << "No reader for content:" << file;
        return false;
    }
    ICatalog *c = rw->readCatalog ( file );
    if ( !c ) {
        debug() << "Unable to create catalog for:" << file;
        return false;
    }
    
    c->setParameter(ICatalogManager::ParameterFile, file);
    c->setParameter(ICatalogManager::ParameterName, name);

    if (readOnly) {
        c->setParameter(ICatalogManager::ParameterReadonly, true);
    } else {
        if (f.permissions() & QFile::WriteUser)
            c->setParameter(ICatalogManager::ParameterReadonly, false);
        else
            c->setParameter(ICatalogManager::ParameterReadonly, true);
    }
    ctlg.append ( c );

    return true;
}


bool CatalogManager::removeCatalog ( const QString& file ) {
    if (file.isEmpty())
        return false;
    for (int i = 0; i < m_ctlg.size(); i++) {
        if (m_ctlg[i] && m_ctlg[i]->parameter(ICatalogManager::ParameterFile) == file) {
            delete m_ctlg.takeAt(i);
            return true;
        }
    }
    return false;
}

bool CatalogManager::load()
{
    loadUserCatalogs();
    loadSystemCatalogs();
    debug() << "Number of user catalogs loaded:" << m_ctlg.size();
    debug() << "Number of system catalogs loaded:" << m_sysctlg.size();
    return true;
}


void CatalogManager::createUserCatalog()
{
    OASISCatalog * catalog = new OASISCatalog();
    catalog->setParameter(ICatalogManager::ParameterFile, catalogDirectory() + "catalog.xml");
    catalog->setParameter(ICatalogManager::ParameterReadonly, false);
    m_ctlg.append(catalog);
}


bool CatalogManager::loadUserCatalogs() {
    QString cf = catalogDirectory() + "config.xml";
    QFile f(cf);
    if (!f.exists()) {
        debug() << "File does not exist: " << f.fileName();
        createUserCatalog();
        return false;
    }
    if (!f.open(QIODevice::ReadOnly)) {
        debug() << "Unable to open file: " << f.fileName();
        createUserCatalog();
        return false;
    }
    QDomDocument doc;
    if (!doc.setContent(&f)) {
        debug() << "Unable to parse file: " << f.fileName();
        createUserCatalog();
        return false;
    }
    QDomNodeList nl = doc.elementsByTagName("catalog");
    for (int i = 0; i < nl.size(); i++) {
        QDomElement e = nl.at(i).toElement();
        if (e.isNull() || e.nodeName() != "catalog")
            continue;
        QString catalog = e.attribute("file");
        QString name = e.attribute("name");
        KUrl url(catalog );
        if (url.isRelative()) {
            KUrl catUrl(cf);
            catalog=QString("%1/%2").arg(catUrl.directory(), catalog);
        }
        if (!addCatalog(m_ctlg, catalog, name, false)) {
            debug() << "Unable load catalog: " << catalog;
            continue;
        }
    }
    if (m_ctlg.isEmpty()) {
        createUserCatalog();
    }
    return true;
}

bool CatalogManager::loadSystemCatalogs() {
    QString cf = KStandardDirs::locate("data", "kdevxmlcatalog/catalogs/config.xml");
    QFile f(cf);
    if (!f.exists()) {
        debug() << "File does not exist: " << f.fileName();
        return false;
    }
    if (!f.open(QIODevice::ReadOnly)) {
        debug() << "Unable to open file: " << f.fileName();
        return false;
    }
    QDomDocument doc;
    if (!doc.setContent(&f)) {
        debug() << "Unable to parse file: " << f.fileName();
        return false;
    }
    QDomNodeList nl = doc.elementsByTagName("catalog");
    for (int i = 0; i < nl.size(); i++) {
        QDomElement e = nl.at(i).toElement();
        if (e.isNull() || e.nodeName() != "catalog")
            continue;
        QString catalog = e.attribute("file");
        QString name = e.attribute("name");
        KUrl url(catalog );
        if (url.isRelative()) {
            KUrl catUrl(cf);
            catalog=QString("%1/%2").arg(catUrl.directory(), catalog);
        }
        if (!addCatalog(m_sysctlg, catalog, name, true)) {
            debug() << "Unable load catalog: " << catalog;
            continue;
        }
    }
    return true;
}

bool CatalogManager::save() const {
    QFile f(catalogDirectory() + "config.xml");
    if (!f.open(QIODevice::WriteOnly)) {
        debug() << "Unable to open file: " << f.fileName();
        return false;
    }
    QTextStream stream(&f);
    stream << "<catalogs>\n";
    foreach(ICatalog *c, m_ctlg) {
        if (!c)
            continue;
        QString catalogFile = c->parameter(ICatalogManager::ParameterFile).toString();
        if (catalogFile.isEmpty()) {
            debug() << "No file parameter for catalog";
            continue;
        }
        stream << QString("  <catalog file=\"%1\"/>\n").arg(catalogFile);
        if (!c->parameter(ICatalogManager::ParameterReadonly).toBool()) {
            writer(c)->writeCatalog(c, catalogFile);
        }
    }
    stream << "</catalogs>\n";
    f.close();
    return true;
}


QStringList CatalogManager::catalogFileList() const
{
    QStringList catalogs;
    foreach(ICatalog *catalog, m_ctlg ) {
        if (!catalog)
            continue;
        catalogs.append(catalog->parameter(ICatalogManager::ParameterFile).toString());
    }
    return catalogs;
}


#include "catalogmanager.moc"

