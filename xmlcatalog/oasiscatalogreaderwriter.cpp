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

#include "oasiscatalogreaderwriter.h"
#include "oasiscatalog.h"
#include "publicid.h"
#include "catalog_debug.h"

#include <QtCore/QFile>

#include <KDE/KUrl>
#include <KDE/KDebug>

#define SCC static const char *
#define SCCI const char * OASISCatalogReaderWriter::
#include <KMessageBox>


struct OASISCatalogReaderWriter::e {
public:
    SCC Catalog;
    SCC DelegatePublic;
    SCC DelegateSystem;
    SCC DelegateURI;
    SCC Group;
    SCC NextCatalog;
    SCC Public;
    SCC RewriteSystem;
    SCC RewriteURI;
    SCC System;
    SCC Doctype;
    SCC SystemSuffix;
    SCC Uri;
    SCC UriSuffix;
};
SCCI e::Catalog = "catalog";
SCCI e::DelegatePublic = "delegatePublic";
SCCI e::DelegateSystem = "delegateSystem";
SCCI e::DelegateURI = "delegateURI";
SCCI e::Group = "group";
SCCI e::NextCatalog = "nextCatalog";
SCCI e::Public = "public";
SCCI e::RewriteSystem = "rewriteSystem";
SCCI e::RewriteURI = "rewriteURI";
SCCI e::System = "system";
SCCI e::Doctype = "doctype";
SCCI e::SystemSuffix = "systemSuffix";
SCCI e::Uri = "uri";
SCCI e::UriSuffix = "uriSuffix";

struct OASISCatalogReaderWriter::a {
public:
    SCC Id;
    SCC Name;
    SCC Prefer;
    SCC PublicId;
    SCC SystemId;
    SCC Uri;
    SCC Base;
    SCC Catalog;
    SCC UriStartString;
    SCC SystemIdStartString;
    SCC RewritePrefix;
    SCC SystemIdSuffix;
    SCC UriSuffix;
    SCC PublicIdStartString;
};
SCCI a::Id = "id";
SCCI a::Name = "name";
SCCI a::Prefer = "prefer";
SCCI a::PublicId = "publicId";
SCCI a::SystemId = "systemId";
SCCI a::Uri = "uri";
SCCI a::Base = "base";
SCCI a::Catalog = "catalog";
SCCI a::UriStartString = "uriStartString";
SCCI a::RewritePrefix = "rewritePrefix";
SCCI a::SystemIdSuffix = "systemIdSuffix";
SCCI a::UriSuffix = "uriSuffix";
SCCI a::SystemIdStartString = "systemIdStartString";
SCCI a::PublicIdStartString = "publicIdStartString";


OASISCatalogReaderWriter::OASISCatalogReaderWriter() {

}


OASISCatalogReaderWriter::~OASISCatalogReaderWriter() {

}


bool OASISCatalogReaderWriter::accepts ( const QString& file ) const {
    QFile f(file);
    if (!f.open(QIODevice::ReadOnly)) {
        debug() << "Unable to open file:" << file;
        return false;
    }
    QDomDocument d;
    if (d.setContent(&f,true))
        return accepts(d);
    return false;
}

bool OASISCatalogReaderWriter::accepts ( const QDomDocument& doc ) const {
    if (doc.isNull())
        return false;
    QDomNodeList nl = doc.elementsByTagName(e::Catalog);
    for (int ni = 0; ni < nl.size(); ni++) {
        QDomElement el = nl.at(ni).toElement();
        //debug() << el.namespaceURI();
        if (el.nodeName() == e::Catalog /*&& PublicId::encodeURN(el.namespaceURI()) == "urn:oasis:names:tc:entity:xmlns:xml:catalog"*/)
            return true;
    }
    return false;
}


ICatalog* OASISCatalogReaderWriter::readCatalog ( const QString& file ) const {
    QFile f(file);
    if (!f.open(QIODevice::ReadOnly)) {
        debug() << "Unable to open file:" << file;
        return 0;
    }
    QDomDocument d;
    if (d.setContent(&f,true)) {
        ICatalog* c = readCatalog(d);
        if (c) {
            //Add the directory as the base path.
            //This should not be saved, so that the catalog may be moved and still work.

            OASISCatalog *catalog = (OASISCatalog *)c;
            if (catalog->m_base.isEmpty()) {
                catalog->m_base = KUrl(file).directory(KUrl::AppendTrailingSlash);
            }

            return c;
        }
    }
    return 0;
}

ICatalog* OASISCatalogReaderWriter::readCatalog ( const QDomDocument& doc ) const {
    if (doc.isNull())
        return 0;
    if (!accepts(doc))
        return 0;
    QDomNodeList nl = doc.elementsByTagName(e::Catalog);
    for (int ni = 0; ni < nl.size(); ni++) {
        QDomElement el = nl.at(ni).toElement();
        if (el.nodeName() == e::Catalog) {
            OASISCatalog * c = new OASISCatalog;
            c->m_base = el.attribute(a::Base);
            c->m_id = el.attribute(a::Id);
            c->m_prefer = el.attribute(a::Prefer);
            if (c->m_prefer.isEmpty())
                c->m_prefer = OASISCatalog::PreferPublic;
            c->m_doc = doc;
            c->setElement(el);
            readNode(nl.at(ni), c);
            return c;
        }
    }
    return 0;
}

void OASISCatalogReaderWriter::readNode ( const QDomNode& node, OASISCatalogContainer* c ) const {
    QDomElement el = node.toElement();
    QDomNodeList nl = el.childNodes();
    for (int ni = 0; ni < nl.size(); ni++) {
        QDomElement el = nl.at(ni).toElement();
        if (el.nodeName() == e::Group) {
            OASISCatalogGroup *grp = new OASISCatalogGroup();
            grp->m_id = el.attribute(a::Id);
            grp->m_prefer = el.attribute(a::Prefer);
            grp->m_base = el.attribute(a::Base);
            grp->m_parent = c;
            c->m_groups.append(grp);
            readNode(nl.at(ni), grp);
            grp->setElement(el);
            /*NOTE A group
            <group
            id = id
            prefer = "system" | "public"
            xml:base = uri-reference>
            <!-- Content: (public | system | rewriteSystem | systemSuffix | delegatePublic | delegateSystem | uri | rewriteURI | uriSuffix | delegateURI | nextCatalog)+ -->
            </group>
            */
        }
        if (el.nodeName() == e::Public) {
            OASISCatalogEntry *en = new OASISCatalogEntry();
            en->m_base = el.attribute(a::Base);
            en->m_id = el.attribute(a::Id);
            //Make sure it is a valid publicId or null
            en->m_publicId = PublicId::decodeURN(el.attribute(a::PublicId));
            en->m_url = el.attribute(a::Uri);
            en->m_parent = c;
            c->m_entries.append(en);
            en->setElement(el);
            /*NOTE A public entry
            <public
            id = id
            publicId = public-identifier
            uri = uri-reference
            xml:base = uri-reference />
            */
        }
        if (el.nodeName() == e::DelegatePublic) {
            OASISCatalogContainer::OASISDelegate d;
            d.id = el.attribute(a::Id);
            d.base = el.attribute(a::Base);
            d.part = el.attribute(a::PublicIdStartString);
            d.catalog = el.attribute(a::Catalog);
            d.element = el;
            c->m_delegatePublic.append(d);
            /*NOTE
            <delegatePublic
            id = id
            publicIdStartString = public-identifier-prefix
            catalog = uri-reference
            xml:base = uri-reference />
            */
        }
        if (el.nodeName() == e::System) {
            OASISCatalogEntry *en = new OASISCatalogEntry();
            en->m_base = el.attribute(a::Base);
            en->m_id = el.attribute(a::Id);
            en->m_systemId = el.attribute(a::SystemId);
            en->m_url = el.attribute(a::Uri);
            en->m_parent = c;
            c->m_entries.append(en);
            en->setElement(el);
            /*NOTE A system entry
            <system
            id = id
            systemId = string
            uri = uri-reference
            xml:base = uri-reference />
            */
        }
        if (el.nodeName() == e::Doctype) {
            OASISCatalogEntry *en = new OASISCatalogEntry();
            en->m_base = el.attribute(a::Base);
            en->m_id = el.attribute(a::Id);
            en->m_doctype = el.attribute(a::Name);
            en->m_url = el.attribute(a::Uri);
            en->m_parent = c;
            c->m_entries.append(en);
            en->setElement(el);
            /*NOTE A doctype entry
            <doctype
            id = id
            name = string
            uri = uri-reference
            xml:base = uri-reference />
            */
        }
        if (el.nodeName() == e::DelegateSystem) {
            OASISCatalogContainer::OASISDelegate d;
            d.id = el.attribute(a::Id);
            d.base = el.attribute(a::Base);
            d.part = el.attribute(a::SystemIdStartString);
            d.catalog = el.attribute(a::Catalog);
            d.element = el;
            c->m_delegateSystem.append(d);
            /*TODO
            <delegateSystem
            id = id
            systemIdStartString = string
            catalog = uri-reference
            xml:base = uri-reference />
            */
        }
        if (el.nodeName() == e::RewriteSystem) {
            OASISCatalogContainer::OASISRewrite r;
            r.id = el.attribute(a::Id);
            r.part = el.attribute(a::SystemIdStartString);
            r.replacement = el.attribute(a::RewritePrefix);
            r.base = el.attribute(a::Base);
            r.element = el;
            c->m_rewriteSystem.append(r);
            /*NOTE
            <rewriteSystem
            id = id
            systemIdStartString = string
            rewritePrefix = uri-reference />
            */
        }
        if (el.nodeName() == e::SystemSuffix) {
            OASISCatalogContainer::OASISRewrite r;
            r.id = el.attribute(a::Id);
            r.part = el.attribute(a::SystemIdSuffix);
            r.replacement = el.attribute(a::Uri);
            r.base = el.attribute(a::Base);
            r.element = el;
            c->m_systemSuffix.append(r);
            /*NOTE
            <systemSuffix
            id = id
            systemIdSuffix = string
            uri = uri-reference
            xml:base = uri-reference />
            */
        }
        if (el.nodeName() == e::NextCatalog) {
            /*TODO load and parse the next catalog
            <nextCatalog
            id = id
            catalog = uri-reference
            xml:base = uri-reference />
            */
        }
        if (el.nodeName() == e::Uri) {
            OASISCatalogEntry *en = new OASISCatalogEntry();
            en->m_base = el.attribute(a::Base);
            en->m_id = el.attribute(a::Id);
            en->m_uri = el.attribute(a::Name);
            en->m_url = el.attribute(a::Uri);
            en->m_parent = c;
            en->setElement(el);
            c->m_entries.append(en);
            /*NOTE URI rewrite
            <uri
            id = id
            name = string
            uri = uri-reference
            xml:base = uri-reference />
            */
        }
        if (el.nodeName() == e::DelegateURI) {
            OASISCatalogContainer::OASISDelegate d;
            d.id = el.attribute(a::Id);
            d.base = el.attribute(a::Base);
            d.part = el.attribute(a::UriStartString);
            d.catalog = el.attribute(a::Catalog);
            d.element = el;
            c->m_delegateUri.append(d);
            /*TODO
            <delegateURI
            id = id
            uriStartString = string
            catalog = uri-reference
            xml:base = uri-reference />
            */
        }
        if (el.nodeName() == e::RewriteURI) {
            OASISCatalogContainer::OASISRewrite r;
            r.id = el.attribute(a::Id);
            r.part = el.attribute(a::UriStartString);
            r.replacement = el.attribute(a::RewritePrefix);
            r.base = el.attribute(a::Base);
            r.element = el;
            c->m_rewriteUri.append(r);
            /*NOTE
            <rewriteURI
            id = id
            uriStartString = string
            rewritePrefix = uri-reference />
            */
        }
        if (el.nodeName() == e::UriSuffix) {
            OASISCatalogContainer::OASISRewrite r;
            r.id = el.attribute(a::Id);
            r.part = el.attribute(a::UriStartString);
            r.replacement = el.attribute(a::RewritePrefix);
            r.base = el.attribute(a::Base);
            r.element = el;
            c->m_uriSuffix.append(r);
            /*NOTE
            <uriSuffix
            id = id
            uriSuffix = string
            uri = uri-reference
            xml:base = uri-reference />
            */
        }
    }
}

bool OASISCatalogReaderWriter::accepts(const ICatalog* catalog) const
{
    return dynamic_cast<const OASISCatalog *>(catalog);
}

bool OASISCatalogReaderWriter::writeCatalog(const ICatalog* catalog, const QString& file) const
{
    const OASISCatalog * c = dynamic_cast<const OASISCatalog *>(catalog);
    if (!c)
        return false;
    if (c->m_doc.isNull())
        return false;

    QFile f(file);
    if (!f.open(QIODevice::WriteOnly)) {
        debug() << "Unable to write to file: " << file;
        return false;
    }

    QTextStream stream(&f);
    c->m_doc.save(stream, 2);
    return true;
}
