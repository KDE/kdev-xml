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

#include "importdeclaration.h"

#include <language/duchain/duchainregister.h>

#include <KLocalizedString>

using namespace Xml;

typedef KDevelop::DeclarationData ImportDeclarationData;

REGISTER_DUCHAIN_ITEM(ImportDeclaration);

ImportDeclaration::ImportDeclaration( const KDevelop::SimpleRange& range, KDevelop::DUContext* parentContext )
    : Declaration( range, parentContext )
{
    setKind(Import);
}

ImportDeclaration::ImportDeclaration( const Xml::ImportDeclaration& rhs )
    : Declaration( rhs )
{
    setKind(Import);
}

ImportDeclaration::ImportDeclaration( KDevelop::DeclarationData& dd )
    : Declaration( dd )
{
    setKind(Import);
}

ImportDeclaration::~ImportDeclaration()
{
}

QString ImportDeclaration::toString() const
{
    return i18n("Import of %1", url().str());
}

KDevelop::IndexedString ImportDeclaration::url() const
{
    return identifier().identifier();
}

KDevelop::Declaration* ImportDeclaration::clonePrivate() const
{
    return new ImportDeclaration(*this);
}