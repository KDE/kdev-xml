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

#include "schemacompletionsupport.h"

#include "../language_debug.h"

#include <KDE/KTextEditor/Document>
#include <KDE/KMimeType>

#include <language/duchain/topducontext.h>
#include <language/duchain/duchainlock.h>
#include <language/duchain/duchain.h>
#include <language/duchain/declaration.h>

#include "schema/schema.h"

using namespace Xml;
using namespace KDevelop;
using namespace KTextEditor;


QList< CompletionItem::Ptr > SchemaCompletionSupport::findEnumerations(
    KTextEditor::Document* document,
    const KTextEditor::Range& range,
    const QString& element,
    const QString& attribute) const
{
    Q_UNUSED(range);
    DUChainReadLocker lock;
    QString attributeName = attribute;
    QString elementName = element;
    QString elementNsPrefix;
    QString attributeNsPrefix;
    QString nsPrefix;
    TopDUContext * tc = 0;
    QList<CompletionItem::Ptr > items;

    //TODO find using the chain
    if (attribute.isEmpty())
        return items;

    tc = DUChain::self()->chainForDocument(document->url());
    if (!tc) return items;

    //Schema's
    /* Scenarios:
    * <element attribute="enum">
    *    element's ns = imported namespace (xmlns)
    *    attribute's ns = imported namespace (xmlns)
    *
    * <prefix:element attribute="enum">
    *    element's ns = prefix namespace (xmlns:prefix)
    *    attributes's ns = prefix namespace (xmlns:prefix)
    *
    * <prefix:element prefixother:attribute="enum">
    *    element's ns = prefix namespace (xmlns:prefix)
    *    attributes's ns = prefixother namespace (xmlns:prefixother)
    */

    if (attributeName.contains(":")) {
        attributeNsPrefix = attributeName.split(':')[0];
        attributeName = attributeName.split(':')[1];
    }

    if (elementName.contains(":")) {
        elementNsPrefix = elementName.split(':')[0];
        elementName = elementName.split(':')[1];
    }

    IndexedString elementNs;
    if (!elementNsPrefix.isEmpty())
        elementNs = IndexedString(findPrefixForNamespace(tc, elementNsPrefix));
    if (elementNs.isEmpty())
        elementNs = IndexedString(findNamespaceUse(tc));

    IndexedString attributeNs;
    if (!attributeNsPrefix.isEmpty())
        attributeNs = IndexedString(findPrefixForNamespace(tc, attributeNsPrefix));
    if (attributeNs.isEmpty())
        attributeNs = elementNs;

    if (!elementNs.isEmpty()) {
        SchemaMutexLocker lock;
        SchemaNodePtr n = SchemaController::self()->node(SchemaQName(IndexedString(elementName), elementNs));
        if (n) {
            debug() << "Completing for namespace:" << elementNs.str();
            SchemaNodePtr a = n->attributes().value(SchemaQName(IndexedString(attributeName), attributeNs));
            if (a) {
                foreach(SchemaNodePtr en, a->enums().values()) {
                    SchemaEnum * e = dynamic_cast<SchemaEnum *>(en.data());
                    if (e)
                        items.append(CompletionItem::Ptr(new CompletionItem(e->value().str(), 10, CompletionItem::Enum)));
                }
            }
        } else {
            debug() << "No node found:" << elementName << elementNs.str();
        }
    }

    return items;
}

QList< CompletionItem::Ptr > SchemaCompletionSupport::findChildElements(
    KTextEditor::Document* document,
    const KTextEditor::Range& range,
    const QString& element,
    const QString& nsPrefix) const
{
    DUChainReadLocker lock;
    DUContext * ctx = 0;
    Declaration *dec = 0;
    QString elementName = element;
    QString elementNsPrefix;
    QString childNsPrefix = nsPrefix;
    TopDUContext * tc = 0;
    QHash<QString, CompletionItem::Ptr > items;

    tc = DUChain::self()->chainForDocument(document->url());
    if (!tc) return items.values();

    //NO? try the chain then
    if (elementName.isEmpty()) {
        ctx = tc->findContextAt(CursorInRevision(range.start().line(), range.start().column()));
        if (!ctx) return items.values();

        dec = ctx->owner();
        if (!dec) return items.values();
        //TODO prettyname
        elementName = dec->identifier().toString();
    }

    //Schema's
    /* Scenarios:
    * <element><child>
    *    element's ns = imported namespace (xmlns)
    *    child's ns = imported namespace (xmlns)
    *
    * <element><prefix:child>
    *    element's ns = imported namespace (xmlns)
    *    child's ns = prefix namespace (xmlns:prefix)
    *
    * <prefix:element><child>
    *    element's ns = prefix namespace (xmlns:prefix)
    *    child's ns = prefix namespace (xmlns:prefix)
    */

    if (elementName.contains(":")) {
        elementNsPrefix = elementName.split(':')[0];
        elementName = elementName.split(':')[1];
    }

    if (childNsPrefix.isEmpty())
        childNsPrefix = elementNsPrefix;

    IndexedString ns;
    if (!childNsPrefix.isEmpty())
        ns = IndexedString(findPrefixForNamespace(tc, childNsPrefix));
    if (ns.isEmpty())
        ns = IndexedString(findNamespaceUse(tc));

    if (!ns.isEmpty()) {
        debug() << "Completing for namespace:" << ns.str();
        SchemaMutexLocker lock;
        SchemaNodePtr n = SchemaController::self()->node(SchemaQName(KDevelop::IndexedString(elementName), ns));
        if (!n)
            n = SchemaNodePtr::dynamicCast<Schema>(SchemaController::self()->schemaForDocument(findImportForNamespace(tc, ns.str())));
        if (n) {
            foreach(SchemaNodePtr e, n->elements().values()) {
                QString prettyName = e->name().str();
                debug() << "findImportForNs:" << findImportForNamespace(tc, e->ns().str()).str();
                SchemaPtr schema = SchemaController::self()->schemaForDocument(findImportForNamespace(tc, e->ns().str()));
                if (schema && schema->qualifiedElements()) {
                    QString prefix = findNamespaceForPrefix(tc, e->ns().str());
                    prettyName = QString("%1:%2").arg(prefix,prettyName);
                } else {
                    debug() << "No schema found";
                }
                items.insert(prettyName, CompletionItem::Ptr(new CompletionItem(prettyName, 10, CompletionItem::Element)));
            }
        } else {
            debug() << "No node found:" << elementName << ns.str();
        }
    } else {
        debug() << "No ns found";
    }

    return items.values();
}


QList< CompletionItem::Ptr > SchemaCompletionSupport::findAttributes(
    KTextEditor::Document* document,
    const KTextEditor::Range& range,
    const QString& element,
    const QString& nsPrefix) const
{
    DUChainReadLocker lock;
    DUContext * ctx = 0;
    Declaration *dec = 0;
    QString elementName = element;
    QString elementNsPrefix;
    QString attributeNsPrefix = nsPrefix;
    TopDUContext * tc = 0;
    QHash< QString, CompletionItem::Ptr > items;

    tc = DUChain::self()->chainForDocument(document->url());
    if (!tc) {
        debug() << "No top context";
        return items.values();
    }

    //NO? try the chain then
    if (elementName.isEmpty()) {
        ctx = tc->findContextAt(CursorInRevision(range.start().line(), range.start().column()));
        if (!ctx) {
            debug() << "No context";
            return items.values();
        }

        dec = ctx->findDeclarationAt(CursorInRevision(range.start().line(), range.start().column()));
        if (!dec) {
            debug() << "No declaration";
            return items.values();
        }
        //TODO prettyname
        elementName = dec->identifier().toString();
    }


    /* Scenarios:
    * <element attribute="enum">
    *    element's ns = imported namespace (xmlns)
    *    attribute's ns = imported namespace (xmlns)
    *
    * <prefix:element attribute="enum">
    *    element's ns = prefix namespace (xmlns:prefix)
    *    attributes's ns = prefix namespace (xmlns:prefix)
    *
    * <prefix:element prefixother:attribute="enum">
    *    element's ns = prefix namespace (xmlns:prefix)
    *    attributes's ns = prefixother namespace (xmlns:prefixother)
    */

    if (elementName.contains(":")) {
        elementNsPrefix = elementName.split(':')[0];
        elementName = elementName.split(':')[1];
    }

    if (attributeNsPrefix.isEmpty())
        attributeNsPrefix = elementNsPrefix;

    IndexedString ns;
    if (!attributeNsPrefix.isEmpty())
        ns = IndexedString(findPrefixForNamespace(tc, attributeNsPrefix));
    if (ns.isEmpty())
        ns = IndexedString(findNamespaceUse(tc));

    if (!ns.isEmpty()) {
        debug() << "Completing for namespace:" << ns.str();
        SchemaMutexLocker lock;
        SchemaNodePtr n = SchemaController::self()->node(SchemaQName(KDevelop::IndexedString(elementName), ns));
        if (!n)
            n = SchemaNodePtr::dynamicCast<Schema>(SchemaController::self()->schemaForDocument(findImportForNamespace(tc, ns.str())));
        if (n) {
            foreach(SchemaNodePtr e, n->attributes().values()) {
                QString prettyName = e->name().str();
                SchemaPtr schema = SchemaController::self()->schemaForDocument(findImportForNamespace(tc, e->ns().str()));
                if (schema && schema->qualifiedAttributes()) {
                    QString prefix = findNamespaceForPrefix(tc, e->ns().str());
                    prettyName = QString("%1:%2").arg(prefix,prettyName);
                }
                items.insert(prettyName, CompletionItem::Ptr(new CompletionItem(prettyName, 0, CompletionItem::Attribute)));
            }
        } else {
            debug() << "No node found:" << elementName << ns.str();
        }
    }
    return items.values();
}
