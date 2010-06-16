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


#include "sgmlcatalogreaderwriter.h"
#include "sgmlcatalog.h"
#include "catalog_debug.h"

#include <QtCore/QFile>
#include <QtCore/QTextStream>
#include <QtCore/QRegExp>
#include <QtCore/QStringList>

#include <KDE/KDebug>
#include "publicid.h"

SGMLCatalogReaderWriter::SGMLCatalogReaderWriter()
{

}

SGMLCatalogReaderWriter::~SGMLCatalogReaderWriter()
{

}

bool SGMLCatalogReaderWriter::accepts(const QString& file) const
{
    return file.toLower().trimmed().endsWith(".cat");
}

bool SGMLCatalogReaderWriter::accepts(const QDomDocument& doc) const
{
    return false;
}

bool SGMLCatalogReaderWriter::accepts(const ICatalog* catalog) const
{
    return dynamic_cast<const SGMLCatalog *>(catalog);
}

ICatalog* SGMLCatalogReaderWriter::readCatalog(const QString& file) const
{
    if (!file.trimmed().toLower().endsWith(".cat"))
        return 0;
    SGMLCatalog * cat = new SGMLCatalog();
    cat->m_file = file;
    readCatalog(file, cat);
    return cat;
}

QString nextWord(const QString & content, uint *lines, uint *charpos) {
    QString word;
#define ci (*charpos)
#define l (*lines)
#define eof (ci >= content.size())
#define next ci++; if(content[ci] == '\n') l++;

    while (!eof) {
        //  --    sdasdsadsa dsa d --\n
        if (content[ci] == '-' && content[ci+1] == '-') {
            next; next;
            while (!eof && !(content[ci] == '-' && content[ci+1] == '-')) {
                next;
            }
            next; next;
            continue;
        }

        if (content[ci].isSpace()) {
            next;
            continue;
        }


        if (content[ci] == '"') {
            next;
            while (!eof && content[ci] != '"') {
                word += content[ci];
                next;
            }
            next;
            return word;
        }

        while (!eof && !content[ci].isSpace() && content[ci] != '"') {
            word += content[ci];
            next;
        }
        return word;
    }
    return word;
}

ICatalog* SGMLCatalogReaderWriter::readCatalog(const QString& file, SGMLCatalog* catalog) const
{
    QFile f(file);
    if(!f.open(QIODevice::ReadOnly)) {
        debug() << "Unable to open file" << file;
        return 0;
    }
    QTextStream stream(&f);
    QString content = stream.readAll();
    catalog->m_fileContents = content;
    f.close();
    uint lines = 0;
    uint charPos = 0;
    while (charPos < content.size()) {
        QStringList list;
        list << nextWord(content, &lines, &charPos);
        if (list[0].toUpper() == "PUBLIC") {
            //PUBLIC urn sysid1
            SGMLCatalogEntry * entry = new SGMLCatalogEntry();
            entry->m_charStart = charPos - 7;
            while (list.size() < 3 && charPos < content.size())
                list << nextWord(content, &lines, &charPos);
            entry->m_charEnd = charPos;
            entry->m_publicId = PublicId::decodeURN(list.size() == 3 ? list[1] : QString());
            entry->m_url = list.size() == 3 ? list[2] : QString();
            entry->m_catalog = catalog;
            catalog->m_entries.append(entry);
            continue;
        }

        if (list[0].toUpper() == "URI") {
            //PUBLIC urn sysid1
            SGMLCatalogEntry * entry = new SGMLCatalogEntry();
            entry->m_charStart = charPos - 4;
            while (list.size() < 3 && charPos < content.size())
                list << nextWord(content, &lines, &charPos);
            entry->m_charEnd = charPos;
            entry->m_uri = list.size() == 3 ? list[1] : QString();
            entry->m_url = list.size() == 3 ? list[2] : QString();
            entry->m_catalog = catalog;
            catalog->m_entries.append(entry);
            continue;
        }

        if (list[0].toUpper() == "SYSTEM") {
            //SYSTEM sysid1 sysid2
            SGMLCatalogEntry * entry = new SGMLCatalogEntry();
            entry->m_charStart = charPos - 7;
            while (list.size() < 3 && charPos < content.size())
                list << nextWord(content, &lines, &charPos);
            entry->m_charEnd = charPos;
            entry->m_systemId = list.size() == 3 ? list[1] : QString();
            entry->m_url = list.size() == 3 ? list[2] : QString();
            entry->m_catalog = catalog;
            catalog->m_entries.append(entry);
            continue;
        }

        if (list[0].toUpper() == "DOCTYPE") {
            //DOCTYPE name sysid
            SGMLCatalogEntry * entry = new SGMLCatalogEntry();
            entry->m_charStart = charPos - 8;
            while (list.size() < 3 && charPos < content.size())
                list << nextWord(content, &lines, &charPos);
            entry->m_charEnd = charPos;
            entry->m_doctype = list.size() == 3 ? list[1] : QString();
            entry->m_url = list.size() == 3 ? list[2] : QString();
            entry->m_catalog = catalog;
            catalog->m_entries.append(entry);
            continue;
        }

        if (list[0].toUpper() == "CATALOG") {
            //CATALOG sysid
            while (list.size() < 2 && charPos < content.size())
                list << nextWord(content, &lines, &charPos);
            debug() << "Multiple catalogs not supported";
            continue;
        }

        if (list[0].toUpper() == "BASE") {
            //BASE sysid
            while (list.size() < 2 && charPos < content.size())
                list << nextWord(content, &lines, &charPos);
            catalog->m_base = list.size() == 2 ? list[1] : QString();
            continue;
        }

        if (list[0].toUpper() == "OVERRIDE") {
            //OVERRIDE bool
            while (list.size() < 2 && charPos < content.size())
                list << nextWord(content, &lines, &charPos);
            catalog->m_override = (list.size() == 2 ? list[1] : QString()).toUpper() == "YES";
            continue;
        }

        debug() << "Not supported: " << list[0];
    }
    return catalog;
}



bool SGMLCatalogReaderWriter::writeCatalog(const ICatalog* catalog, const QString& file) const
{
    const SGMLCatalog * cat = dynamic_cast<const SGMLCatalog *>(catalog);
    if (!cat)
        return false;
    //TODO keep catalog format
    QFile f(file);
    if(!f.open(QIODevice::WriteOnly)) {
        debug() << "Unable to open file" << file;
        return false;
    }
    QTextStream stream(&f);

    QString contents = cat->m_fileContents;
    
    foreach(SGMLCatalogEntry *entry, cat->m_entries) {
        if (!entry)
            continue;
        if(entry->m_deleted) {
            contents.replace(entry->m_charStart,
                             entry->m_charEnd - entry->m_charStart,
                             "\n");
            continue;
        }
        if (!entry->publicId().isEmpty()) {
            QString text = QString("PUBLIC \"%1\" \"%2\"\n").arg(entry->publicId(), entry->URL());
            if(entry->m_charStart < 0)
                contents.append(text);
            /*else
                contents.replace(entry->m_charStart,
                             entry->m_charEnd - entry->m_charStart,
                             text);
            */
        }
        if (!entry->systemId().isEmpty()) {
            QString text = QString("SYSTEM \"%1\" \"%2\"\n").arg(entry->systemId(), entry->URL());
            if(entry->m_charStart < 0)
                contents.append(text);
            /*else
            contents.replace(entry->m_charStart,
                             entry->m_charEnd - entry->m_charStart,
                             text);
            */
        }
        if (!entry->doctype().isEmpty()) {
            QString text = QString("DOCTYPE \"%1\" \"%2\"\n").arg(entry->doctype(), entry->URL());
            if(entry->m_charStart < 0)
                contents.append(text);
            /*else
            contents.replace(entry->m_charStart,
                             entry->m_charEnd - entry->m_charStart,
                             text);
            */
        }
        if (!entry->URI().isEmpty()) {
            QString text = QString("URI \"%1\" \"%2\"\n").arg(entry->URI(), entry->URL());
            if(entry->m_charStart < 0)
                contents.append(text);
            /*else
            contents.replace(entry->m_charStart,
                             entry->m_charEnd - entry->m_charStart,
                             text);
            */
        }
    }

    stream << contents;

    return true;
}



