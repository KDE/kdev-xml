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


#include "oasiscatalog.h"
#include "publicid.h"
#include "catalog_debug.h"

#include <KDE/KUrl>
#include <KDE/KMessageBox>

const QString OASISCatalogContainer::PreferSystem = "system";
const QString OASISCatalogContainer::PreferPublic = "public";

const QString OASISCatalogContainer::ParameterBase = "xml:base";
const QString OASISCatalogContainer::ParameterId = "id";
const QString OASISCatalogContainer::ParameterPrefer = "prefer";

const QString OASISCatalogEntry::ParameterUrl = "uri";
const QString OASISCatalogEntry::ParameterBase = "xml:base";
const QString OASISCatalogEntry::ParameterId = "id";


IOASISCatalogDomNode::~IOASISCatalogDomNode() {}

IOASISCatalogDomNode::IOASISCatalogDomNode() {}


OASISCatalogEntry::OASISCatalogEntry() {
    m_parent = 0;
    m_deleted = false;
}

OASISCatalogEntry::~OASISCatalogEntry() {}

ICatalogGroup* OASISCatalogEntry::group() const {
    return dynamic_cast<ICatalogGroup*> ( m_parent );
}

ICatalog* OASISCatalogEntry::catalog() const {
    ICatalogGroup* g = dynamic_cast<ICatalogGroup*> ( m_parent );
    if ( g )
        return g->catalog();
    return dynamic_cast<ICatalog*> ( m_parent );
}

const QString& OASISCatalogEntry::base() const {
    if ( !m_base.isEmpty() )
        return m_base;
    if ( m_parent )
        return m_parent->base();
    return m_base;
}

QVariant OASISCatalogEntry::parameter(const QString& key) const
{
    if (key == ParameterBase)
        return base();
    if (key == ParameterId)
        return id();
    if (key == ParameterUrl)
        return URL();
    return Parameterized< QString, QVariant >::parameter(key);
}

void OASISCatalogEntry::setParameter(const QString& key, const QVariant& value)
{
    if (key == ParameterBase)
        setBase(value.toString());
    if (key == ParameterId)
        setId(value.toString());
    if (key == ParameterUrl)
        setURL(value.toString());
    Parameterized< QString, QVariant >::setParameter(key, value);
}

OASISCatalogGroup::OASISCatalogGroup() {}

OASISCatalogGroup::~OASISCatalogGroup() {}

ICatalog* OASISCatalogGroup::catalog() const {
    ICatalogGroup* g = dynamic_cast<ICatalogGroup*> ( m_parent );
    if ( g )
        return g->catalog();
    return dynamic_cast<ICatalog*> ( m_parent );
}

QVariant OASISCatalogContainer::parameter(const QString& key) const
{
    if (key == ParameterBase)
        return m_base;
    if (key == ParameterId)
        return m_id;
    if (key == ParameterPrefer)
        return m_prefer;
    return Parameterized< QString, QVariant >::parameter(key);
}

void OASISCatalogContainer::setParameter(const QString& key, const QVariant& value)
{
    if (key == ParameterBase)
        setBase(value.toString());
    if (key == ParameterId)
        setId(value.toString());
    if (key == ParameterPrefer)
        setPrefer(value.toString());
    Parameterized< QString, QVariant >::setParameter(key, value);
}


ICatalogEntry* OASISCatalogContainer::addPublicEntry(const QString& publicId,
                                                     const QString& url,
                                                     const QHash< QString,
                                                     QVariant >& parameters)
{
    OASISCatalogEntry *entry = new OASISCatalogEntry();
    entry->m_parent = this;
    entry->m_publicId = publicId;
    entry->m_systemId  = QString::null;
    entry->m_uri  = QString::null;
    entry->m_url = url;
    foreach(QString key, parameters.keys()) {
        entry->setParameter(key, parameters[key]);
    }
    m_entries.append(entry);
    if (!element().isNull()) {
        QDomElement e = element().ownerDocument().createElement("public");
        if (!entry->m_id.isNull())
            e.setAttribute("id", entry->m_id);
        if (!entry->m_base.isNull())
            e.setAttribute("xml:base", entry->m_base);
        e.setAttribute("publicId", entry->m_publicId);
        e.setAttribute("uri", entry->m_url);
        element().appendChild(e);
        entry->setElement(e);
    }
    return entry;
}

ICatalogEntry* OASISCatalogContainer::addSystemEntry(const QString& systemId,
                                                     const QString& url,
                                                     const QHash< QString,
                                                     QVariant >& parameters)
{
    OASISCatalogEntry *entry = new OASISCatalogEntry();
    entry->m_parent = this;
    entry->m_publicId = QString::null;
    entry->m_systemId = systemId;
    entry->m_uri = QString::null;
    entry->m_url = url;
    foreach(QString key, parameters.keys()) {
        entry->setParameter(key, parameters[key]);
    }
    m_entries.append(entry);
    if (!element().isNull()) {
        QDomElement e = element().ownerDocument().createElement("system");
        if (!entry->m_id.isNull())
            e.setAttribute("id", entry->m_id);
        if (!entry->m_base.isNull())
            e.setAttribute("xml:base", entry->m_base);
        e.setAttribute("systemId", entry->m_systemId);
        e.setAttribute("uri", entry->m_url);
        element().appendChild(e);
        entry->setElement(e);
    }
    return entry;
}

ICatalogEntry* OASISCatalogContainer::addUriEntry(const QString& uri,
                                                  const QString& url,
                                                  const QHash< QString,
                                                  QVariant >& parameters)
{
    OASISCatalogEntry *entry = new OASISCatalogEntry();
    entry->m_parent = this;
    entry->m_publicId = QString::null;
    entry->m_systemId = QString::null;
    entry->m_uri = uri;
    entry->m_url = url;
    foreach(QString key, parameters.keys()) {
        entry->setParameter(key, parameters[key]);
    }
    m_entries.append(entry);
    if (!element().isNull()) {
        QDomElement e = element().ownerDocument().createElement("uri");
        if (!entry->m_id.isNull())
            e.setAttribute("id", entry->m_id);
        if (!entry->m_base.isNull())
            e.setAttribute("xml:base", entry->m_base);
        e.setAttribute("name", entry->m_uri);
        e.setAttribute("uri", entry->m_url);
        element().appendChild(e);
        entry->setElement(e);
    }
    return entry;
}

ICatalogEntry* OASISCatalogContainer::addDoctypeEntry(const QString& doctype, const QString& url, const QHash< QString, QVariant >& parameters)
{
    OASISCatalogEntry *entry = new OASISCatalogEntry();
    entry->m_parent = this;
    entry->m_publicId = QString::null;
    entry->m_systemId = QString::null;
    entry->m_uri = QString::null;
    entry->m_doctype = doctype;
    entry->m_url = url;
    foreach(QString key, parameters.keys()) {
        entry->setParameter(key, parameters[key]);
    }
    m_entries.append(entry);
    if (!element().isNull()) {
        QDomElement e = element().ownerDocument().createElement("doctype");
        if (!entry->m_id.isNull())
            e.setAttribute("id", entry->m_id);
        if (!entry->m_base.isNull())
            e.setAttribute("xml:base", entry->m_base);
        e.setAttribute("name", entry->m_doctype);
        e.setAttribute("uri", entry->m_url);
        element().appendChild(e);
        entry->setElement(e);
    }
    return entry;
}

void OASISCatalogContainer::removeEntry(ICatalogEntry* entry)
{
    if(!entry)
        return;
    foreach(ICatalogEntry *e, entries()) {
        if(!e)
            continue;
        if (e == entry) {
            OASISCatalogEntry *ent = static_cast<OASISCatalogEntry *>(e);
            if (!ent)
                continue;
            if (ent->parent() != this) {
                ent->parent()->removeEntry(ent);
            } else {
                if (!ent->element().isNull() && !ent->element().parentNode().isNull())
                    ent->element().parentNode().removeChild(ent->element());
                m_entries.removeAll(ent);
                ent->m_deleted = true;
            }
        }
    }
}

QList< ICatalogEntry* > OASISCatalogContainer::entries() const {
    QList< ICatalogEntry* > ret;
    foreach(OASISCatalogEntry *e, m_entries) {
        if(e && !e->m_deleted)
            ret.append(e);
    }
    foreach(OASISCatalogGroup *g, m_groups)
        foreach(ICatalogEntry *entry, g->entries()) {
            OASISCatalogEntry *e = dynamic_cast<OASISCatalogEntry *>(entry);
            if(e && !e->m_deleted)
                ret.append(e);
        }
    return ret;
}

QList< ICatalogGroup* > OASISCatalogContainer::groups() const {
    QList< ICatalogGroup* > ret;
    foreach(OASISCatalogGroup *g, m_groups)
    ret.append(g);
    return ret;
}

OASISCatalogContainer::OASISCatalogContainer() {
    m_parent = 0;
    m_prefer = PreferPublic;
}

OASISCatalogContainer::~OASISCatalogContainer() {
    foreach ( OASISCatalogEntry *e, m_entries ) {
        if ( e ) delete e;
    }
    foreach ( OASISCatalogGroup *g, m_groups ) {
        if ( g ) delete g;
    }
//    foreach ( OASISCatalogContainer *c, m_catalogs ) {
//        if ( c ) delete c;
//    }
    m_parent = 0;
}


const QString& OASISCatalogContainer::base() const {
    if ( m_base.isEmpty() && m_parent)
        return m_parent->base();
    return m_base;
}

const QString& OASISCatalogContainer::prefer() const {
    if ( m_prefer.isEmpty() && m_parent )
        return m_parent->prefer();
    return m_prefer;
}


QString OASISCatalogContainer::resolve ( const QString& publicId, const QString& systemId ) const {
    QString res;
    if (prefer() == PreferPublic && !publicId.isEmpty()) {
        res = resolvePublicId(publicId);
        if(!res.isEmpty())
            return res;
        else if (!systemId.isEmpty())
            return resolveSystemId(systemId);
    }
    else if (!systemId.isEmpty())
        return resolveSystemId(systemId);
    return QString::null;
}

QString OASISCatalogContainer::resolvePublicId ( const QString& publicId ) const {
    QString id = PublicId::decodeURN ( publicId );
    foreach ( OASISCatalogEntry *e, m_entries ) {
        if (!e || e->m_deleted) continue;
        if ( !e->publicId().isEmpty() && ( e->publicId() == publicId || e->publicId() == id ) ) {
            QString url = e->URL();
            KUrl u(url);
            if (u.isValid() && u.isRelative()) {
                QString b = e->base();
                if (!b.isEmpty())
                    b = base();
                url = b + url;
            }
            return url;
        }
    }
    foreach ( OASISCatalogGroup *g, m_groups ) {
        if (!g) continue;
        QString res = g->resolvePublicId(publicId);
        if (!res.isEmpty())
            return res; //Already rewritten
    }
    //TODO implement delegate
    
    return QString::null;
}


QString OASISCatalogContainer::resolveSystemId ( const QString& systemId ) const {
    QString id = PublicId::decodeURN ( systemId );
    foreach ( OASISCatalogEntry *e, m_entries ) {
        if (!e || e->m_deleted) continue;
        if ( !e->systemId().isEmpty() && e->systemId() == id ) {
            QString url = e->URL();
            KUrl u(url);
            if (u.isValid() && u.isRelative()) {
                QString b = e->base();
                if (!b.isEmpty())
                    b = base();
                url = b + url;
            }
            return url;
        }
    }
    foreach ( OASISCatalogGroup *g, m_groups ) {
        if (!g) continue;
        QString res = g->resolveSystemId(systemId);
        if (!res.isEmpty())
            return res; //Already rewritten
    }
    //TODO implement delegate
    
    return QString::null;
}

QString OASISCatalogContainer::resolveDoctype(const QString& doctype) const
{
    foreach ( OASISCatalogEntry *e, m_entries ) {
        if (!e || e->m_deleted) continue;
        if ( !e->doctype().isEmpty() && e->doctype().toLower() == doctype.toLower() ) {
            QString url = e->URL();
            KUrl u(url);
            if (u.isValid() && u.isRelative()) {
                QString b = e->base();
                if (!b.isEmpty())
                    b = base();
                url = b + url;
            }
            return url;
        }
    }
    foreach ( OASISCatalogGroup *g, m_groups ) {
        if (!g) continue;
        QString res = g->resolveDoctype(doctype);
        if (!res.isEmpty())
            return res; //Already rewritten
    }
    //TODO implement delegate
    
    return QString::null;
}


QString OASISCatalogContainer::rewriteSystemId ( const QString & systemId ) const {
    if ( systemId.isEmpty() )
        return systemId;

    QString res = systemId;

    OASISRewrite tmp;
    OASISRewrite &match = tmp;
    int matchLen = 0;
    for (int i = 0; i < m_rewriteSystem.size(); i++) {
        const OASISRewrite &r = m_rewriteSystem[i];
        if ( res.startsWith ( r.part ) ) {
            int len = PublicId::normalize ( r.part ).length();
            if ( len > matchLen ) {
                matchLen = len;
                match = r;
            }
        }
    }
    //with rewrite it is a replacement
    if ( matchLen > 0 ) {
        res = res.replace ( match.part, match.replacement );
        KUrl url ( res );
        if ( url.isValid() && url.isRelative() ) {
            QString b = match.base;
            if ( b.isEmpty() )
                b = base();
            res = b + res;
        }
        return res;
    }

    if ( matchLen < 1 ) {
        for (int i = 0; i < m_systemSuffix.size(); i++) {
            const OASISRewrite &r = m_systemSuffix[i];
            if ( res.endsWith ( r.part ) ) {
                int len = PublicId::normalize ( r.part ).length();
                if ( len > matchLen ) {
                    matchLen = len;
                    match = r;
                }
            }
        }
    }
    //with suffix it is the whole uri (match.replacement) part
    if ( matchLen > 0 ) {
        res = match.replacement;
        KUrl url ( res );
        if ( url.isValid() && url.isRelative() ) {
            QString b = match.base;
            if ( b.isEmpty() )
                b = base();
            res = b + res;
        }
        return res;
    }

    if (matchLen < 1) {
        foreach ( OASISCatalogGroup *g, m_groups ) {
            if (!g) continue;
            res = g->rewriteSystemId(systemId);
            if (!res.isEmpty())
                return res;
        }
    }

    return QString::null;
}

QString OASISCatalogContainer::resolveUri(const QString& uri) const
{
    QString id = PublicId::decodeURN ( uri );
    foreach ( OASISCatalogEntry *e, m_entries ) {
        if (!e|| e->m_deleted) continue;
        if ( !e->URI().isEmpty() && e->URI() == id ) {
            QString url = e->URL();
            KUrl u(url);
            if (u.isValid() && u.isRelative()) {
                QString b = e->base();
                if (!b.isEmpty())
                    b = base();
                url = b + url;
            }
            return url;
        }
    }
    foreach ( OASISCatalogGroup *g, m_groups ) {
        if (!g) continue;
        QString res = g->resolveUri(uri);
        if (!res.isEmpty())
            return res; //Already rewritten
    }
    return QString::null;
}

QString OASISCatalogContainer::rewriteUri ( const QString & uri ) const {
    if ( uri.isEmpty() )
        return uri;

    QString res = uri;

    OASISRewrite tmp;
    OASISRewrite &match = tmp;
    int matchLen = 0;
    for (int i = 0; i < m_rewriteUri.size(); i++) {
        const OASISRewrite &r = m_rewriteUri[i];
        if ( res.startsWith ( r.part ) ) {
            int len = PublicId::normalize ( r.part ).length();
            if ( len > matchLen ) {
                matchLen = len;
                match = r;
            }
        }
    }
    //with rewrite it is a replacement
    if ( matchLen > 0 ) {
        res = res.replace ( match.part, match.replacement );
        KUrl url ( res );
        if ( url.isValid() && url.isRelative() ) {
            QString b = match.base;
            if ( b.isEmpty() )
                b = base();
            res = b + res;
        }
        return res;
    }

    if ( matchLen < 1 ) {
        for (int i = 0; i < m_uriSuffix.size(); i++) {
            const OASISRewrite &r = m_uriSuffix[i];
            if ( res.endsWith ( r.part ) ) {
                int len = PublicId::normalize ( r.part ).length();
                if ( len > matchLen ) {
                    matchLen = len;
                    match = r;
                }
            }
        }
    }
    //with suffix it is the whole uri (match.replacement) part
    if ( matchLen > 0 ) {
        res = match.replacement;
        KUrl url ( res );
        if ( url.isValid() && url.isRelative() ) {
            QString b = match.base;
            if ( b.isEmpty() )
                b = base();
            res = b + res;
        }
        return res;
    }

    if (matchLen < 1) {
        foreach ( OASISCatalogGroup *g, m_groups ) {
            if (!g) continue;
            res = g->rewriteUri(uri);
            if (!res.isEmpty())
                return res;
        }
    }

    return QString::null;
}


OASISCatalog::OASISCatalog() {
    m_parent = 0;
    QString content = "<!DOCTYPE catalog PUBLIC \"-//OASIS//DTD XML Catalogs V1.0//EN\"\
    \"http://www.oasis-open.org/committees/entity/release/1.0/catalog.dtd\">\n\
    <catalog xmlns=\"urn:oasis:names:tc:entity:xmlns:xml:catalog\" prefer=\"public\"></catalog>";
    m_doc.setContent(content);
    m_element = m_doc.elementsByTagName("catalog").at(0).toElement();

}

OASISCatalog::~OASISCatalog() {

}

QString OASISCatalog::resolve ( const QString& publicId, const QString& systemId ) const {
    return OASISCatalogContainer::resolve ( publicId, systemId );
}

QString OASISCatalog::resolvePublicId ( const QString& publicId ) const {
    return OASISCatalogContainer::resolvePublicId ( publicId );
    //TODO delegate
}

QString OASISCatalog::resolveSystemId ( const QString& systemId ) const {
    QString res = OASISCatalogContainer::resolveSystemId ( systemId );
    if (res.isEmpty())
        res = rewriteSystemId(systemId);
    //TODO delegate
    return res;
}

QString OASISCatalog::resolveUri(const QString& uri) const
{
    QString res = OASISCatalogContainer::resolveUri ( uri );
    if (res.isEmpty())
        res = rewriteUri(uri);
    //TODO delegate
    return res;
}

QString OASISCatalog::resolveDoctype(const QString& doctype) const
{
    return OASISCatalogContainer::resolveDoctype ( doctype );
}



