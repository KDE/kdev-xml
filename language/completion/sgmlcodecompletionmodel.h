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
#include "completionbase.h"


#include <KTextEditor/CodeCompletionModel>
#include <KTextEditor/CodeCompletionModelControllerInterface>
#include <language/codecompletion/codecompletionmodel.h>
#include <language/codecompletion/codecompletionworker.h>

namespace KDevelop {
class TopDUContext;
}

namespace Xml {

class KDEVSGMLCOMPLETION_EXPORT SgmlCodeCompletionModel  :
    public KTextEditor::CodeCompletionModel2,
    public KTextEditor::CodeCompletionModelControllerInterface3,
    public CompletionModelBase
{
    Q_OBJECT
    Q_INTERFACES(KTextEditor::CodeCompletionModelControllerInterface3)
public:
    SgmlCodeCompletionModel(QObject *parent);
    virtual ~SgmlCodeCompletionModel();

    virtual void completionInvoked(KTextEditor::View* view, const KTextEditor::Range& range, InvocationType invocationType);

    virtual QVariant data(const QModelIndex & index, int role) const;

    //virtual QMap< int, QVariant > itemData(const QModelIndex& index) const;

    virtual KTextEditor::Range completionRange(KTextEditor::View* view, const KTextEditor::Cursor &position);

    virtual bool shouldAbortCompletion(KTextEditor::View* view, const KTextEditor::Range& range, const QString &currentCompletion);

    virtual bool shouldStartCompletion(KTextEditor::View* view, const QString& insertedText, bool userInsertion, const KTextEditor::Cursor& position);

    virtual void executeCompletionItem2(KTextEditor::Document* document, const KTextEditor::Range& word, const QModelIndex& index) const;

protected:
       /** The item and its MatchQuality 0-10*/
    QList< CompletionItem::Ptr > m_items;
    
    //Set to true if lowercase is used
    int m_formatCase;

    /** The  depth to indent by*/
    int m_depth;

    /** @todo get users indentation profile */
    QString getIndentstring(KTextEditor::Document* document, int depth) const;
    
    QString formatSource(KTextEditor::Document* document, const QString& source, const KTextEditor::Cursor& pos) const;

    /** @todo get users preferences for HTML ie: lower-case */
    QString formatItem(KTextEditor::Document* document, const QString &str, CompletionItem::CompletionItemType type) const;

    /** Finds the seperator before the cursor.
     */
    QChar getSeperator(KTextEditor::Document* document, const KTextEditor::Cursor &position) const;

    KTextEditor::Range growRangeLeft(KTextEditor::Document* document, const KTextEditor::Range& range, const QString &condition, bool ignoreWhites=false) const;

    KTextEditor::Range growRangeRight(KTextEditor::Document* document, const KTextEditor::Range& range, const QString &condition, bool ignoreWhites=false) const;

};

}

#endif // SGMLCODECOMPLETIONMODEL_H
