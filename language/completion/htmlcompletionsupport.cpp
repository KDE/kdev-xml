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

#include "htmlcompletionsupport.h"
#include <KTextEditor/Document>
#include <QtCore/QFile>
#include <QtCore/QDir>
#include <KIconLoader>
#include <KMimeType>

using namespace Xml;

class HtmlCompletionItem : public CompletionItem {
public:
    enum IconType { FOLDER, IMAGE, FILE };

    HtmlCompletionItem(QString itemName,
                       int itemMatchLevel,
                       CompletionItemType itemType,
                       IconType iconType) :
            CompletionItem(itemName, itemMatchLevel, itemType),
            m_iconType(iconType) {
    }

    /* The icons is only 12px and too small for detail
     * so a minimum of types will do
     */
    virtual const QIcon getIcon() const {
        static QHash < IconType, QIcon> icons;
        if ( icons.contains ( m_iconType ) )
            return icons[m_iconType];
        QString name;
        if (m_iconType == FOLDER)
            name = "folder";
        else if (m_iconType == IMAGE)
            name = "image";
        else if (m_iconType == FILE)
            name = "file";
        if (name.isEmpty())
            return QIcon();
        QIcon i ( QString ( ":/xml/completion/%1.png" ).arg ( name ) );
        icons.insert ( m_iconType, i );
        return icons[m_iconType];
    }

    IconType m_iconType;
};


QList< CompletionItem::Ptr > HtmlCompletionSupport::findEnumerations(KTextEditor::Document* document,
        const KTextEditor::Range& range,
        const QString& element,
        const QString& attribute) const
{
    QList<CompletionItem::Ptr> items;
    if (document->mimeType() != "text/html") {
        return items;
    }

    //Meta
    if ((element.toLower() == "meta" && attribute.toLower() == "name")) {
        items.append(CompletionItem::Ptr(new CompletionItem("description", 0, CompletionItem::Enum)));
        items.append(CompletionItem::Ptr(new CompletionItem("keywords", 0, CompletionItem::Enum)));
        items.append(CompletionItem::Ptr(new CompletionItem("author", 0, CompletionItem::Enum)));
        items.append(CompletionItem::Ptr(new CompletionItem("copyright", 0, CompletionItem::Enum)));
        items.append(CompletionItem::Ptr(new CompletionItem("robots", 0, CompletionItem::Enum)));
        items.append(CompletionItem::Ptr(new CompletionItem("googlebot", 0, CompletionItem::Enum)));
    }

    if ((element.toLower() == "meta" && attribute.toLower() == "http-equiv")) {
        items.append(CompletionItem::Ptr(new CompletionItem("cache-control", 0, CompletionItem::Enum)));
        items.append(CompletionItem::Ptr(new CompletionItem("content-language", 0, CompletionItem::Enum)));
        items.append(CompletionItem::Ptr(new CompletionItem("content-type", 0, CompletionItem::Enum)));
        items.append(CompletionItem::Ptr(new CompletionItem("expires", 0, CompletionItem::Enum)));
        items.append(CompletionItem::Ptr(new CompletionItem("pragma", 0, CompletionItem::Enum)));
        items.append(CompletionItem::Ptr(new CompletionItem("refresh", 0, CompletionItem::Enum)));
    }

    //Mime types
    if ((element.toLower() == "script" && attribute.toLower() == "type")
        || (element.toLower() == "link" && attribute.toLower() == "type")
        || (element.toLower() == "a" && attribute.toLower() == "type")
        || (element.toLower() == "object" && attribute.toLower() == "type")
        || (element.toLower() == "object" && attribute.toLower() == "codetype")
        || (element.toLower() == "param" && attribute.toLower() == "type")
        || (element.toLower() == "form" && attribute.toLower() == "type")
        || (element.toLower() == "form" && attribute.toLower() == "accept")
        || (element.toLower() == "input" && attribute.toLower() == "accept")
        || (element.toLower() == "style" && attribute.toLower() == "type")) {
        KMimeType::List list = KMimeType::allMimeTypes();
        foreach(KMimeType::Ptr mime, list) {
            items.append(CompletionItem::Ptr(new CompletionItem(mime->name(), 0, CompletionItem::Enum)));
        }
        if(element.toLower() == "script")
            items.append(CompletionItem::Ptr(new CompletionItem("text/javascript", 10, CompletionItem::Enum)));
        if(element.toLower() == "link" || element.toLower() == "style")
            items.append(CompletionItem::Ptr(new CompletionItem("text/css", 10, CompletionItem::Enum)));
        if(element.toLower() == "form")
            items.append(CompletionItem::Ptr(new CompletionItem("application/x-www-form-urlencoded", 10, CompletionItem::Enum)));
    }

    //Files
    if ((element.toLower() == "img" && attribute.toLower() == "src")
            || (element.toLower() == "script" && attribute.toLower() == "src")
            || (element.toLower() == "input" && attribute.toLower() == "src")
            || (element.toLower() == "base" && attribute.toLower() == "href")
            || (element.toLower() == "frame" && attribute.toLower() == "src")
            || (element.toLower() == "iframe" && attribute.toLower() == "src")
            || (element.toLower() == "form" && attribute.toLower() == "action")
            || (element.toLower() == "link" && attribute.toLower() == "href")
            || (element.toLower() == "a" && attribute.toLower() == "href")
            || (element.toLower() == "area" && attribute.toLower() == "href")
            || (element.toLower() == "body" && attribute.toLower() == "background")) {

        KTextEditor::Range r = range;
        r.end().setColumn(r.start().column());

        //We want to know the complete path before the cursor
        QString seps = "\"'=\r\n\t<>";
        const QString line = document->line(range.start().line());
        for (int i = range.start().column() - 1; i >= 0; --i) {
            if (seps.contains(line[i]))
                break;
            r.start().setColumn(i);
        }

        //Find a directory the user is in, this coule be none, relative or absolute
        QString path = document->text(r).trimmed();
        bool relative = false;
        if (path.isEmpty()) {
            path = document->url().directory();
        } else {
            if (KUrl(path).isRelative() || path.startsWith(".")) {
                if (!path.endsWith("/")) {
                    path = path.mid(0, path.lastIndexOf('/'));
                }
                path = document->url().directory() + "/" + path;
            } else {
                if (!path.endsWith("/")) {
                    path = path.mid(0, path.lastIndexOf('/'));
                }
            }
        }
        QDir dir(path);

        //Create list of files
        QFileInfoList list = dir.entryInfoList();
        foreach(QFileInfo info, list) {
            if (info.fileName().startsWith("./")
                    || info.fileName() == "."
                    || info.fileName().contains("/./")
                    || info.fileName().endsWith("~"))
                continue;
            if (info.isDir()) {
                QString f = info.fileName() + "/";
                if (relative)
                    f = f.replace(document->url().directory() + "/", "");
                items.append(CompletionItem::Ptr(new HtmlCompletionItem(f, 0, CompletionItem::Other, HtmlCompletionItem::FOLDER)));
            } else {
                QString suffix = info.suffix().toLower();
                QStringList extensions = QString("svg svgz png jpg jpeg jpe tga tif tiff bmp gif").split(" ");
                if (extensions.contains(suffix)) {
                    QString f = info.fileName();
                    if(element.toLower() == "img"
                        || element.toLower() == "body"
                        || element.toLower() == "input")
                        items.append(CompletionItem::Ptr(new HtmlCompletionItem(f, 10, CompletionItem::Other, HtmlCompletionItem::IMAGE)));
                    else
                        items.append(CompletionItem::Ptr(new HtmlCompletionItem(f, 0, CompletionItem::Other, HtmlCompletionItem::IMAGE)));
                } else {
                    QString f = info.fileName();
                    items.append(CompletionItem::Ptr(new HtmlCompletionItem(f, 0, CompletionItem::Other, HtmlCompletionItem::FILE)));
                }
            }
            KIconLoader loader;
        }
    }

    return items;
}
