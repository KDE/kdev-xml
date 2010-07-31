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

#include "elementdeclaration.h"
#include <language/duchain/duchainregister.h>

namespace Xml {

using namespace KDevelop;

REGISTER_DUCHAIN_ITEM(ElementDeclaration);
DEFINE_LIST_MEMBER_HASH(ElementDeclarationData, m_attributes, IndexedString);

ElementDeclarationData::ElementDeclarationData()
{
    initializeAppendedLists();
    closeTagRequired = true;
    openTagRequired = true;
    elementType = Element;
}

ElementDeclarationData::ElementDeclarationData(const Xml::ElementDeclarationData& rhs): ClassDeclarationData(rhs)
{
    initializeAppendedLists();
    copyListsFrom(rhs);
    closeTagRequired = rhs.closeTagRequired;
    openTagRequired = rhs.openTagRequired;
    name = rhs.name;
    contentType = rhs.contentType;
    elementType = rhs.elementType;
}

ElementDeclarationData::~ElementDeclarationData()
{
    freeAppendedLists();
}

ElementDeclaration::ElementDeclaration(const ElementDeclaration& rhs): ClassDeclaration(rhs)
{
    setSmartRange(rhs.smartRange(), DocumentRangeObject::DontOwn);
}

ElementDeclaration::ElementDeclaration(const SimpleRange& range, DUContext* context):
        ClassDeclaration(*new ElementDeclarationData, range, context)
{
    d_func_dynamic()->setClassId(this);
    if (context)
        setContext(context);
}

ElementDeclaration::ElementDeclaration(ElementDeclarationData& data): ClassDeclaration(data)
{
    d_func_dynamic()->setClassId(this);
}

ElementDeclaration::ElementDeclaration(ElementDeclarationData& data, const SimpleRange& range, DUContext* context): ClassDeclaration(data, range, context)
{
    d_func_dynamic()->setClassId(this);
}

ElementDeclaration::~ElementDeclaration()
{

}

QString ElementDeclaration::prettyName() const
{
    if(!d_func()->namespacePrefix.isEmpty())
        return QString("%1:%2").arg(d_func()->namespacePrefix.str(), d_func()->name.str());
    else  if(!d_func()->name.isEmpty())
        return d_func()->name.str();
    return "";
}


IndexedString ElementDeclaration::name() const
{
    return d_func()->name;
}

void ElementDeclaration::setName(const IndexedString& n)
{
    d_func_dynamic()->name = n;
}

void ElementDeclaration::setCloseTagRequired(bool required)
{
    d_func_dynamic()->closeTagRequired = required;
}

bool ElementDeclaration::closeTagRequired() const
{
    return d_func()->closeTagRequired;
}

void ElementDeclaration::setOpenTagRequired(bool required)
{
    d_func_dynamic()->openTagRequired = required;
}

bool ElementDeclaration::openTagRequired() const
{
    return d_func()->openTagRequired;
}

void ElementDeclaration::setContentType(const IndexedString& ct)
{
    d_func_dynamic()->contentType = ct;
}

IndexedString ElementDeclaration::contentType() const
{
    return d_func()->contentType;
}

void ElementDeclaration::setContent(const IndexedString& ct)
{
    d_func_dynamic()->content = ct;
}

IndexedString ElementDeclaration::content() const
{
    return d_func()->content;
}

void ElementDeclaration::setNamespacePrefix(const IndexedString& ns)
{
    d_func_dynamic()->namespacePrefix = ns;
}

IndexedString ElementDeclaration::namespacePrefix() const
{
    return d_func()->namespacePrefix;
}

ElementDeclarationData::ElementType ElementDeclaration::elementType() const
{
    return d_func()->elementType;
}

void ElementDeclaration::setElementType(ElementDeclarationData::ElementType type)
{
    d_func_dynamic()->elementType = type;
}

QString ElementDeclaration::toString() const
{
    return ClassDeclaration::toString();
}


void ElementDeclaration::addAttributes(const IndexedString& str)
{
    d_func_dynamic()->m_attributesList().append(str);
}

const IndexedString* ElementDeclaration::attributes() const
{
    return d_func()->m_attributes();
}

unsigned int ElementDeclaration::attributesSize() const
{
    return d_func()->m_attributesSize();
}

void ElementDeclaration::clearAttributes()
{
    d_func_dynamic()->m_attributesList().clear();
}

Declaration* ElementDeclaration::clonePrivate() const
{
    return new ElementDeclaration(*this);
}

}
