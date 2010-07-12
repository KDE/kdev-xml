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

#ifndef SGMLCOMPLETIONSUPPORT_H
#define SGMLCOMPLETIONSUPPORT_H

#include "completionitem.h"

#include <KTextEditor/Range>
#include <QtGui/QIcon>

namespace KDevelop {
class TopDUContext;
class IndexedString;
}

namespace Xml {

/** Finds data for the completion model.
 *  To add completion support overide this.
 *  @note Instances must be added to CompletionBase
 */
class KDEVSGMLCOMPLETION_EXPORT CompletionSupport
{
public:
    virtual ~CompletionSupport() {}

    /** Find all the children of an element / namespace
     *  @param document The document completing for
     *  @param range The range we are completing / replacing
     *  @param element This must be parents pretty name ie including the ns prefix, DTD does not use ns prefix's but may define a name like 'xs:element'
     *  @param nsPrefix The user may want to complete for only a specific namespace, the prefix is resolved to a namespace using the DUChain
     *  @return A list of CompletionItem's
     */
    virtual QList< CompletionItem::Ptr > findChildElements(KTextEditor::Document* document,
            const KTextEditor::Range& range,
            const QString &element = QString(),
            const QString &nsPrefix = QString()) const {
        Q_UNUSED(range);
        Q_UNUSED(element);
        Q_UNUSED(document);
        Q_UNUSED(nsPrefix);
        return QList<CompletionItem::Ptr>();
    }

    /** Find all the attributes of an element / namespace
     *  @param document The document completing for
     *  @param range The range we are completing / replacing
     *  @param element This must be parents pretty name ie including the ns prefix, DTD does not use ns prefix's but may define a name like 'xs:element'
     *  @param nsPrefix The user may want to complete for only a specific namespace, the prefix is resolved to a namespace using the DUChain
     *  @return A list of CompletionItem's
     */
    virtual QList< CompletionItem::Ptr > findAttributes(KTextEditor::Document* document,
            const KTextEditor::Range& range,
            const QString &element = QString(),
            const QString &nsPrefix = QString()) const {
        Q_UNUSED(range);
        Q_UNUSED(element);
        Q_UNUSED(document);
        Q_UNUSED(nsPrefix);
        return QList<CompletionItem::Ptr>();
    }

    /** Find all the enumerations of an attribute
     *  @param document The document completing for
     *  @param range The range we are completing / replacing
     *  @param element This must be parents pretty name ie including the ns prefix, DTD does not use ns prefix's but may define a name like 'xs:element'
     *  @param attribute The attribute to list enumerations for, must be the pretty name.
     *  @return A list of CompletionItem's
     */
    virtual QList< CompletionItem::Ptr > findEnumerations(KTextEditor::Document* document,
            const KTextEditor::Range& range,
            const QString &element = QString(),
            const QString &attribute = QString()) const {
        Q_UNUSED(range);
        Q_UNUSED(element);
        Q_UNUSED(document);
        Q_UNUSED(attribute);
        return QList<CompletionItem::Ptr>();
    }

    /** Find all elements
     *  Only searches the DUChain not Schema.
     *  @param document The document completing for
     *  @param range The range we are completing / replacing
     */
    virtual QList< CompletionItem::Ptr > findAll(KTextEditor::Document* document,
            const KTextEditor::Range& range) const {
        Q_UNUSED(range);
        Q_UNUSED(document);
        return QList<CompletionItem::Ptr>();
    }

    /** Find all entities
     *  Only searches the DUChain not Schema.
     *  @param document The document completing for
     *  @param range The range we are completing / replacing
     */
    virtual QList< CompletionItem::Ptr > findAllEntities(KTextEditor::Document* document,
            const KTextEditor::Range& range) const  {
        Q_UNUSED(range);
        Q_UNUSED(document);
        return QList<CompletionItem::Ptr>();
    }

    virtual QList< CompletionItem::Ptr > findHeadersForDocument(KTextEditor::Document *document) const  {
        Q_UNUSED(document);
        return QList<CompletionItem::Ptr>();
    }

    /** Find the namespaece for a prefix (alias) ie: xmlns:prefix="namespace"
     *  @note The DUChain must be locked
     */
    QString findPrefixForNamespace(KDevelop::TopDUContext *topContext, const QString &nsPrefix) const;

    /** Find the prefix for a namespace (alias) ie: xmlns:prefix="namespace"
     *  @note The DUChain must be locked
     */
    QString findNamespaceForPrefix(KDevelop::TopDUContext *topContext, const QString &ns) const;

    /** The namespace being used ie: xmlns="namespace"
     *  @note The DUChain must be locked
     */
    QString findNamespaceUse(KDevelop::TopDUContext *topContext) const;
    
    /** Find the imported document for a namespace ie: xmlns:prefix="namespace"
     *  Beacause you can have multiple schema documents for a namespace you need to know which one was included.
     *  Imported contexts is searched for a namespace declaration and the matching document is returned. 
     *  @note The DUChain must be locked
     */
    KDevelop::IndexedString findImportForNamespace(KDevelop::TopDUContext *tc, const QString &ns) const;
};

}

#endif // SGMLCOMPLETIONSUPPORT_H
