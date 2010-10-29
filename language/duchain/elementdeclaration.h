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

#ifndef ELEMENTDECLARATION_H
#define ELEMENTDECLARATION_H

#include "duchainexport.h"
#include <language/duchain/classdeclaration.h>
#include <language/duchain/declaration.h>
#include <language/duchain/declarationdata.h>

namespace Xml {

using namespace KDevelop;

KDEVSGMLDUCHAIN_EXPORT DECLARE_LIST_MEMBER_HASH(ElementDeclarationData, m_attributes, IndexedString)

class KDEVSGMLDUCHAIN_EXPORT ElementDeclarationData : public ClassDeclarationData
{
public:

    enum ElementType {
        Element, //An element <ns:name ... > </ns:name>
        CloseTag, //An child to an element marks the end of the context of an element
        Text, //A text element
        Doctype,
        CDATA, //A cdata element
        PCDATA, //A pcdata element
        Entity, //A entity
        Processing //A processing instruction
    };

    ElementDeclarationData();

    ElementDeclarationData(const ElementDeclarationData& rhs);

    ~ElementDeclarationData();

    IndexedString name;
    IndexedString namespacePrefix;
    IndexedString contentType;
    IndexedString content;
    ElementType elementType;
    bool closeTagRequired : 1;
    bool openTagRequired : 1;

    START_APPENDED_LISTS_BASE(ElementDeclarationData, ClassDeclarationData);
    APPENDED_LIST_FIRST(ElementDeclarationData, IndexedString, m_attributes);
    END_APPENDED_LISTS(ElementDeclarationData, m_attributes);
};


class KDEVSGMLDUCHAIN_EXPORT ElementDeclaration : public ClassDeclaration
{
public:

    ElementDeclaration(const ElementDeclaration &rhs);
    ElementDeclaration(const RangeInRevision& range, DUContext* context);
    ElementDeclaration(ElementDeclarationData &data);
    ElementDeclaration(ElementDeclarationData &data, const RangeInRevision& range, DUContext* context);
    ~ElementDeclaration();

    virtual QString toString() const;

    QString prettyName() const;

    void setName(const IndexedString &n);
    void setName(const QString &n) {
        setName(IndexedString(n));
    }

    IndexedString name() const;

    void setNamespacePrefix(const IndexedString &ns);
    void setNamespacePrefix(const QString &ns) {
        setNamespacePrefix(IndexedString(ns));
    }

    IndexedString namespacePrefix() const;

    void setContentType(const IndexedString &ct);
    void setContentType(const QString &ct) {
        setContentType(IndexedString(ct));
    }
    IndexedString contentType() const;

    void setContent(const IndexedString &ct);
    void setContent(const QString &ct) {
        setContent(IndexedString(ct));
    }
    IndexedString content() const;

    void setElementType(ElementDeclarationData::ElementType type);
    ElementDeclarationData::ElementType elementType() const;

    bool closeTagRequired() const;
    void setCloseTagRequired(bool required);

    bool openTagRequired() const;
    void setOpenTagRequired(bool required);

    virtual const IndexedString* attributes() const;
    virtual unsigned int attributesSize() const;
    virtual void addAttributes(const IndexedString& str);
    virtual void clearAttributes();

    enum {
        Identity = 90
    };

private:
    virtual Declaration* clonePrivate () const;
    DUCHAIN_DECLARE_DATA(ElementDeclaration)
};

}

#endif // ELEMENTDECLARATION_H
