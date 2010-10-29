/***************************************************************************
 *   This file is part of KDevelop                                         *
 *   Copyright 2010 Milian Wolff <mail@milianw.de>                         *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Library General Public License as       *
 *   published by the Free Software Foundation; either version 2 of the    *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this program; if not, write to the                 *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#ifndef XML_IMPORTDECLARATION_H
#define XML_IMPORTDECLARATION_H

#include <language/duchain/declaration.h>
#include <language/duchain/declarationdata.h>

#include "duchainexport.h"

namespace Xml
{

/**
 * NOTE: This is one big hack to get context browsing work a bit better for DTDs
 * See notes in declarationbuilder.cpp
 */
class KDEVSGMLDUCHAIN_EXPORT ImportDeclaration : public KDevelop::Declaration
{

public:
    ImportDeclaration( const KDevelop::RangeInRevision& range, KDevelop::DUContext* parentContext );
    ImportDeclaration(const ImportDeclaration& rhs);
    ImportDeclaration( KDevelop::DeclarationData & dd );
    virtual ~ImportDeclaration();

    virtual QString toString() const;
    virtual KDevelop::IndexedString url() const;

    enum {
        Identity = 91
    };

private:
    virtual Declaration* clonePrivate () const;
    DUCHAIN_DECLARE_DATA(KDevelop::Declaration)
};

}

#endif // XML_IMPORTDECLARATION_H
