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

#ifndef SGMLCODECOMPLETIONMODEL_H
#define SGMLCODECOMPLETIONMODEL_H

#include "completionexport.h"

#include <KTextEditor/CodeCompletionModel>
#include <KTextEditor/CodeCompletionModelControllerInterface>
#include <language/codecompletion/codecompletionmodel.h>
#include <language/codecompletion/codecompletionworker.h>


namespace Xml {

    class KDEVSGMLCOMPLETION_EXPORT SgmlCodeCompletionModel  : public KTextEditor::CodeCompletionModel2, public KTextEditor::CodeCompletionModelControllerInterface
{
    Q_OBJECT
    Q_INTERFACES(KTextEditor::CodeCompletionModelControllerInterface)
public:
    SgmlCodeCompletionModel(QObject *parent);
    virtual ~SgmlCodeCompletionModel();
    
    virtual void completionInvoked(KTextEditor::View* view, const KTextEditor::Range& range, InvocationType invocationType);

    virtual QVariant data(const QModelIndex & index, int role) const;

    //virtual QMap< int, QVariant > itemData(const QModelIndex& index) const;

    virtual KTextEditor::Range completionRange(KTextEditor::View* view, const KTextEditor::Cursor &position);
    
    virtual bool shouldAbortCompletion(KTextEditor::View* view, const KTextEditor::SmartRange& range, const QString &currentCompletion);

    virtual bool shouldStartCompletion(KTextEditor::View* view, const QString& insertedText, bool userInsertion, const KTextEditor::Cursor& position);

    virtual void executeCompletionItem2(KTextEditor::Document* document, const KTextEditor::Range& word, const QModelIndex& index) const;

protected:
    enum CompletionItemType {
        Other,
        Element,
        Attribute,
        Entity,
        Header,
        Enum
    };
    
    typedef struct CompletionItem {
        CompletionItem(QString itemName, int itemMatchLevel, CompletionItemType itemType, bool empty = false) {
            type = itemType;
            matchLevel = itemMatchLevel;
            name = itemName;
            info = "";
            isEmpty = empty;
        }
        CompletionItem(QString itemName, QString itemInfo, int itemMatchLevel, CompletionItemType itemType, bool empty = false) {
            type = itemType;
            matchLevel = itemMatchLevel;
            name = itemName;
            info = itemInfo;
            isEmpty = empty;
        }
        QString name;
        QString info;
        int matchLevel;
        bool isEmpty;
        CompletionItemType type;
    } CompletionItem;

    /** The item and its MatchQuality 0-10*/
    QList< CompletionItem > m_items;

    /** The  depth to indent by*/
    int m_depth;
    
    QList< CompletionItem > findChildElements(KTextEditor::Document* document, const KTextEditor::Range& range, const QString &element = QString()) const;
    QList< CompletionItem > findAttributes(KTextEditor::Document* document, const KTextEditor::Range& range, const QString &element = QString()) const;
    QList< CompletionItem > findAll(KTextEditor::Document* document, const KTextEditor::Range& range) const;
    QList< CompletionItem > findAllEntities(KTextEditor::Document* document, const KTextEditor::Range& range) const;
    QList< CompletionItem > findHeadersForDocument(KTextEditor::Document *document) const;
    
    const QIcon getIcon ( CompletionItemType type ) const;
    
    QString formatSource(KTextEditor::Document* document, const QString& source, const KTextEditor::Cursor& pos) const;
    
    /** @todo get users preferences for HTML ie: lower-case */
    QString formatItem(KTextEditor::Document* document, const QString &str, CompletionItemType type) const;
    
    /** Finds the seperator before the cursor.
     */ 
    QChar getSeperator(KTextEditor::Document* document, const KTextEditor::Cursor &position) const;
    
    KTextEditor::Range growRangeLeft(KTextEditor::Document* document, const KTextEditor::Range& range, const QString &condition) const;
    KTextEditor::Range growRangeRight(KTextEditor::Document* document, const KTextEditor::Range& range, const QString &condition) const;
};

}

#endif // SGMLCODECOMPLETIONMODEL_H
