/*****************************************************************************
 * Copyright (c) 2009 Ruan Strydom <rm3dom@gmail.com>                        *
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

#include "idocumentcachemanager.h"
#include "documentcachemanager.h"

#include <QtGui/QApplication>

#include <KDE/KGlobal>
#include <KDE/KComponentData>
#include <KDE/KStandardDirs>

IDocumentCacheManager * IDocumentCacheManager::instance = new DocumentCacheManager();


IDocumentCacheManager::IDocumentCacheManager() : QObject(QApplication::instance())
{
    Q_ASSERT(instance == 0);
    instance = this;
}

IDocumentCacheManager::~IDocumentCacheManager()
{
    instance = 0;
}

IDocumentCacheManager* IDocumentCacheManager::self()
{
    return instance;
}

QString IDocumentCacheManager::getCacheDirectory() const
{
    return KGlobal::mainComponent().dirs()->saveLocation( "data", "kdevxmlcatalog/cache", true );
}
