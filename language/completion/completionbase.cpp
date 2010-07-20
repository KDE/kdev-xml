/*****************************************************************************
 * Copyright (c) 2010 Ruan Strydom <rm3dom@gmail.com>                        *
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


#include "completionbase.h"

#include "elementdeclaration.h"
#include "completionvisitor.h"

#include <schema/schema.h>

#include <QtGui/QIcon>

#include <KDE/KTextEditor/Document>
#include <KDE/KMimeType>

#include "dtdcompletionsupport.h"
#include "xsdcompletionsupport.h"
#include "schemacompletionsupport.h"
#include "htmlcompletionsupport.h"

using namespace Xml;
using namespace KDevelop;
using namespace KTextEditor;

CompletionModelBase::CompletionModelBase()
{
    m_supportHelpers.append(new DtdCompletionSupport);
    m_supportHelpers.append(new SchemaCompletionSupport);
    m_supportHelpers.append(new XsdCompletionSupport);
    m_supportHelpers.append(new HtmlCompletionSupport);
}

CompletionModelBase::~CompletionModelBase()
{
    foreach(CompletionSupport * s, m_supportHelpers) {
        delete s;
    }
}

QList< CompletionItem::Ptr > CompletionModelBase::findAll(
    KTextEditor::Document* document,
    const KTextEditor::Range& range) const
{
    QList<CompletionItem::Ptr> items;
    foreach(CompletionSupport *s, m_supportHelpers) {
        items.append(s->findAll(document, range));
    }
    return items;
}

QList< CompletionItem::Ptr > CompletionModelBase::findAttributes(
    KTextEditor::Document* document,
    const KTextEditor::Range& range,
    const QString &element,
    const QString &nsPrefix
) const
{
    QList<CompletionItem::Ptr> items;
    foreach(CompletionSupport *s, m_supportHelpers) {
        items.append(s->findAttributes(document, range, element, nsPrefix));
    }
    return items;
}


QList< CompletionItem::Ptr > CompletionModelBase::findChildElements(
    KTextEditor::Document* document,
    const KTextEditor::Range& range,
    const QString &element,
    const QString &nsPrefix
) const
{
    QList<CompletionItem::Ptr> items;
    foreach(CompletionSupport *s, m_supportHelpers) {
        items.append(s->findChildElements(document, range, element, nsPrefix));
    }
    return items;
}

QList< CompletionItem::Ptr > CompletionModelBase::findEnumerations(
    Document* document,
    const KTextEditor::Range& range,
    const QString& element,
    const QString& attribute) const
{
    QList<CompletionItem::Ptr> items;
    foreach(CompletionSupport *s, m_supportHelpers) {
        items.append(s->findEnumerations(document, range, element, attribute));
    }
    return items;
}


QList< CompletionItem::Ptr > CompletionModelBase::findHeadersForDocument(Document* document) const
{
    KMimeType::Ptr mime = KMimeType::mimeType(document->mimeType());
    QList< CompletionItem::Ptr > items;

    if (mime->is("application/xhtml+xml")
            || mime->is("application/docbook+xml")
            || mime->is("application/xml")
            || mime->is("application/xslt+xml")
            || mime->is("application/xsd")
            || mime->is("application/wsdl+xml")
            || mime->is("application/x-wsdl")) {
        items.append(CompletionItem::Ptr(new CompletionItem("<?xml version=\"1.0\" encoding=\"UTF-8\"?>", 0, CompletionItem::Header)));
    }

    foreach(CompletionSupport *s, m_supportHelpers) {
        items.append(s->findHeadersForDocument(document));
    }

    return items;
}

QList< CompletionItem::Ptr > CompletionModelBase::findAllEntities(Document* document, const KTextEditor::Range& range) const
{
    QList<CompletionItem::Ptr> items;
    foreach(CompletionSupport *s, m_supportHelpers) {
        items.append(s->findAllEntities(document, range));
    }
    return items;
}
