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

#ifndef DTDCOMPLETIONSUPPORT_H
#define DTDCOMPLETIONSUPPORT_H

#include "completionsupport.h"

namespace Xml {

/** Provides support for SGML/XML using imported dtd's
*/
class DtdCompletionSupport : public CompletionSupport
{
public:
    virtual QList< CompletionItem::Ptr > findAttributes(KTextEditor::Document* document,
                                                   const KTextEditor::Range& range,
                                                   const QString& element = QString(),
                                                   const QString& nsPrefix = QString()) const;

    virtual QList< CompletionItem::Ptr > findChildElements(KTextEditor::Document* document,
                                                      const KTextEditor::Range& range,
                                                      const QString& element = QString(),
                                                      const QString& nsPrefix = QString()) const;

    virtual QList< CompletionItem::Ptr > findAllEntities(KTextEditor::Document* document,
                                                    const KTextEditor::Range& range) const;

    virtual QList< CompletionItem::Ptr > findAll(KTextEditor::Document* document,
                                            const KTextEditor::Range& range) const;

    virtual QList< CompletionItem::Ptr > findHeadersForDocument(KTextEditor::Document* document) const;
};

}

#endif // DTDCOMPLETIONSUPPORT_H
