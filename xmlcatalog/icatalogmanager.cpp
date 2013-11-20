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

#include "icatalogmanager.h"
#include "catalogmanager.h"

#include <QtGui/QApplication>
#include <QtCore/QFile>

#include <KDE/KUrl>
#include <KDE/KGlobal>
#include <KDE/KComponentData>
#include <KDE/KStandardDirs>
#include <KDE/KIO/CopyInfo>

const QString ICatalogManager::ParameterFile = "catalog.manager.file";
const QString ICatalogManager::ParameterReadonly = "catalog.manager.readonly";
const QString ICatalogManager::ParameterName = "catalog.manager.name";

ICatalogManager* ICatalogManager::instance = 0;

ICatalogManager::ICatalogManager() : QObject(QApplication::instance()) {
    Q_ASSERT(instance == 0);
    instance = this;
}

ICatalogManager::~ICatalogManager() {
    instance = 0;
}

ICatalogManager* ICatalogManager::self() {
    if (!instance) {
        instance = new CatalogManager();
    }

    return instance;
}

QString ICatalogManager::catalogDirectory() const {
    return KGlobal::mainComponent().dirs()->saveLocation( "data", "kdevxmlcatalog", true );
}


#include "icatalogmanager.moc"


