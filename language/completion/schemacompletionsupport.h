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

#ifndef SCHEMACOMPLETIONSUPPORT_H
#define SCHEMACOMPLETIONSUPPORT_H

#include "completionsupport.h"

namespace Xml {

/** Provides support for XML using imported schemas
 */
class SchemaCompletionSupport : public CompletionSupport
{
public:
    virtual QList< CompletionItem::Ptr > findEnumerations(KTextEditor::Document* document,
                                                     const KTextEditor::Range& range,
                                                     const QString& element = QString(),
                                                     const QString& attribute = QString()) const;

    virtual QList< CompletionItem::Ptr > findChildElements(KTextEditor::Document* document, const KTextEditor::Range& range, const QString& element = QString(), const QString& nsPrefix = QString()) const;

    virtual QList< CompletionItem::Ptr > findAttributes(KTextEditor::Document* document, const KTextEditor::Range& range, const QString& element = QString(), const QString& nsPrefix = QString()) const;
};

}

#endif // SCHEMACOMPLETIONSUPPORT_H
