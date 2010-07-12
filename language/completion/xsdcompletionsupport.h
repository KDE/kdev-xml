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

#ifndef XSDCOMPLETIONSUPPORT_H
#define XSDCOMPLETIONSUPPORT_H

#include "completionsupport.h"

namespace KDevelop {
class IndexedString;
}

namespace Xml {

/** Provides support for the XSD file type
 *  This completes types and bases types for the user. 
 */
class XsdCompletionSupport : public CompletionSupport
{
public:
    virtual QList< CompletionItem::Ptr > findEnumerations(KTextEditor::Document* document,
                                                     const KTextEditor::Range& range,
                                                     const QString& element = QString(),
                                                     const QString& attribute = QString()) const;
protected:
    

};

}

#endif // XSDCOMPLETIONSUPPORT_H
