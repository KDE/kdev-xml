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


#ifndef SGMLCOMPLETIONBASE_H
#define SGMLCOMPLETIONBASE_H

#include "completionitem.h"
#include "completionsupport.h"

#include <KTextEditor/Range>
#include <QtGui/QIcon>

namespace KDevelop {
class TopDUContext;
}

namespace Xml {

class KDEVSGMLCOMPLETION_EXPORT CompletionModelBase : public CompletionSupport
{
protected:
    CompletionModelBase();
    virtual ~CompletionModelBase();

    /** Find all the children of an element / namespace
     *  @param document The document completing for
     *  @param range The range we are completing / replacing
     *  @param element This must be parents pretty name ie including the ns prefix, DTD does not use ns prefix's but may define a name like 'xs:element'
     *  @param nsPrefix The user may want to complete for only a specific namespace, the prefix is resolved to a namespace using the DUChain
     *  @return A list of CompletionItem's
     */
    QList< CompletionItem::Ptr > findChildElements(KTextEditor::Document* document,
            const KTextEditor::Range& range,
            const QString &element = QString(),
            const QString &nsPrefix = QString()) const;

    /** Find all the attributes of an element / namespace
     *  @param document The document completing for
     *  @param range The range we are completing / replacing
     *  @param element This must be parents pretty name ie including the ns prefix, DTD does not use ns prefix's but may define a name like 'xs:element'
     *  @param nsPrefix The user may want to complete for only a specific namespace, the prefix is resolved to a namespace using the DUChain
     *  @return A list of CompletionItem's
     */
    QList< CompletionItem::Ptr > findAttributes(KTextEditor::Document* document,
                                           const KTextEditor::Range& range,
                                           const QString &element = QString(),
                                           const QString &nsPrefix = QString()) const;

    /** Find all the enumerations of an attribute
     *  @param document The document completing for
     *  @param range The range we are completing / replacing
     *  @param element This must be parents pretty name ie including the ns prefix, DTD does not use ns prefix's but may define a name like 'xs:element'
     *  @param attribute The attribute to list enumerations for, must be the pretty name.
     *  @return A list of CompletionItem's
     */
    QList< CompletionItem::Ptr > findEnumerations(KTextEditor::Document* document,
            const KTextEditor::Range& range,
            const QString &element = QString(),
            const QString &attribute = QString()) const;

    /** Find all elements
     *  Only searches the DUChain not Schema.
     *  @param document The document completing for
     *  @param range The range we are completing / replacing
     */
    QList< CompletionItem::Ptr > findAll(KTextEditor::Document* document, const KTextEditor::Range& range) const;
    
    /** Find all entities
     *  Only searches the DUChain not Schema.
     *  @param document The document completing for
     *  @param range The range we are completing / replacing
     */
    QList< CompletionItem::Ptr > findAllEntities(KTextEditor::Document* document, const KTextEditor::Range& range) const;
    
    QList< CompletionItem::Ptr > findHeadersForDocument(KTextEditor::Document *document) const;

private:
    QList<CompletionSupport *> m_supportHelpers;
};

}

#endif // SGMLCOMPLETIONBASE_H
