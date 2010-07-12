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

#ifndef COMPLETIONITEM_H
#define COMPLETIONITEM_H

#include <QtCore/QString>

#include "completionexport.h"
#include <KDE/KSharedPtr>
#include <QtGui/QIcon>

class QModelIndex;

namespace KTextEditor {
class Document;
class Range;
}

namespace Xml {

class KDEVSGMLCOMPLETION_EXPORT CompletionItem : public KShared {
public:

    typedef KSharedPtr<CompletionItem> Ptr;
    
    enum CompletionItemType {
        Other,
        Element,
        Attribute,
        Entity,
        Header,
        Enum,
        Custom //Used for custom completion
    };

    CompletionItem(){m_type = Other; m_matchLevel = 0; m_empty=false;}
    virtual ~CompletionItem(){}

    CompletionItem(QString itemName, int itemMatchLevel, CompletionItemType itemType) {
        m_type = itemType;
        m_matchLevel = itemMatchLevel;
        m_name = itemName;
        m_info = "";
    }
    
    CompletionItem(QString itemName, int itemMatchLevel, CompletionItemType itemType, bool empty) {
        m_type = itemType;
        m_matchLevel = itemMatchLevel;
        m_name = itemName;
        m_info = "";
        m_empty = empty;
    }

    CompletionItem(QString itemName, QString itemInfo, int itemMatchLevel, CompletionItemType itemType) {
        m_type = itemType;
        m_matchLevel = itemMatchLevel;
        m_name = itemName;
        m_info = itemInfo;
    }

    virtual inline QString getName() const {
        return m_name;
    }

    virtual inline QString getInfo() const {
        return m_info;
    }

    virtual inline int getMatchLevel() const {
        return m_matchLevel;
    }

    virtual inline CompletionItemType getType() const {
        return m_type;
    }
    
    virtual inline bool empty() const {
        return m_empty;
    }
    
    virtual const QIcon getIcon ( ) const;
    
protected:
    QString m_name;
    /**Additional info*/
    QString m_info;
    /**0-10 where 10 is the best match*/
    int m_matchLevel;
    /**When DTD specifies the element empty*/
    bool m_empty;
    /**The type of completion to perform*/
    CompletionItemType m_type;
};

class KDEVSGMLCOMPLETION_EXPORT CustomCompletionItem : public CompletionItem {
public:
    CustomCompletionItem() {
        m_type = Custom;
    }
    
    virtual ~CustomCompletionItem() {}

    /** Completes / replaces the text in the document.
     *  Overide this to implement custom completion, and ensure that m_type = Custom.
     */
    virtual void complete(KTextEditor::Document* document, const KTextEditor::Range& word) = 0; 
};

}

#endif // COMPLETIONITEM_H
