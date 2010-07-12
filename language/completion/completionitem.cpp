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

#include "completionitem.h"

#include <QtCore/QHash>

using namespace Xml;

const QIcon CompletionItem::getIcon ( ) const
{
    static QHash <CompletionItem::CompletionItemType, QIcon> icons;
    if ( icons.contains ( getType() ) )
        return icons[getType()];
    QString name;
    if (getType() == CompletionItem::Element)
        name = "element";
    else if (getType() == CompletionItem::Attribute)
        name = "attribute";
    else if (getType() == CompletionItem::Entity)
        name = "entity";
    else if (getType() == CompletionItem::Enum)
        name = "enumeration";
    if (name.isEmpty())
        return QIcon();
    QIcon i ( QString ( ":/xml/completion/%1.png" ).arg ( name ) );
    icons.insert ( getType(), i );
    return icons[getType()];
}
