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

#ifndef ABSTRACTBUILDER_H
#define ABSTRACTBUILDER_H

#include "duchainexport.h"

#include "sgmldebugvisitor.h"

#include <language/interfaces/iproblem.h>

namespace Xml
{
class KDEVSGMLDUCHAIN_EXPORT AbstractBuilder: public DefaultVisitor
{
public:
    typedef struct IncludeIdentifier {
        KDevelop::IndexedString doctype;    /*ie html*/
        KDevelop::IndexedString publicId;   /*ie urn*/
        KDevelop::IndexedString systemId;   /*ie uri*/
        KDevelop::IndexedString uri;        /*ie namespaces uri*/
        KDevelop::IndexedString prefix;     /*associated xml ns prefix*/
        bool isNull() {
            return doctype.isEmpty()
                && publicId.isEmpty()
                && systemId.isEmpty()
                && uri.isEmpty()
                && prefix.isEmpty();
        }
    } IncludeIdentifier;
    
    AbstractBuilder() {}
    ~AbstractBuilder() {}

    virtual QList<KDevelop::ProblemPointer> problems() {
        return m_problems;
    }

protected:
    virtual void reportProblem(KDevelop::ProblemData::Severity, AstNode* ast, const QString& message) = 0;
    QList<KDevelop::ProblemPointer> m_problems;
    QMap<QString, IncludeIdentifier> m_entities;
};
}

#endif //ABSTRACTBUILDER_H
