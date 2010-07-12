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

#include "xsdcompletionsupport.h"

#include "../language_debug.h"

#include <KDE/KTextEditor/Document>
#include <KDE/KMimeType>

#include <language/duchain/topducontext.h>
#include <language/duchain/duchainlock.h>
#include <language/duchain/duchain.h>
#include <language/duchain/declaration.h>
#include <language/duchain/classdeclaration.h>
#include <language/duchain/namespacealiasdeclaration.h>

#include "schema/schema.h"

namespace Xml {
class XsdCompletionItem : public CompletionItem {
public:
    XsdCompletionItem(QString itemName,
                      int itemMatchLevel,
                      CompletionItemType itemType,
                      SchemaNode::SchemaNodeType nodeType) :
            CompletionItem(itemName, itemMatchLevel, itemType),
            m_nodeType(nodeType) {
    }
    virtual const QIcon getIcon() const {
        static QHash < SchemaNode::SchemaNodeType, QIcon> icons;
        if ( icons.contains ( m_nodeType ) )
            return icons[m_nodeType];
        QString name;
        if (m_nodeType == SchemaNode::Element)
            name = "element";
        else if (m_nodeType == SchemaNode::Attribute)
            name = "attribute";
        else if (m_nodeType == SchemaNode::AttributeGroup)
            name = "attributeGroup";
        else if (m_nodeType == SchemaNode::Group)
            name = "group";
        else if (m_nodeType == SchemaNode::SimpleType)
            name = "simpleType";
        else if (m_nodeType == SchemaNode::ComplexType)
            name = "complexType";
        else if (m_nodeType == SchemaNode::Enum)
            name = "enumeration";
        else if (m_nodeType == SchemaNode::Schema)
            name = "schema";
        if (name.isEmpty())
            return QIcon();
        QIcon i ( QString ( ":/xml/completion/%1.png" ).arg ( name ) );
        icons.insert ( m_nodeType, i );
        return icons[m_nodeType];
    }
    SchemaNode::SchemaNodeType m_nodeType;
};
}

using namespace Xml;
using namespace KDevelop;
using namespace KTextEditor;


QList< CompletionItem::Ptr > XsdCompletionSupport::findEnumerations(
    KTextEditor::Document* document,
    const KTextEditor::Range& range,
    const QString& element,
    const QString& attribute) const
{
    Q_UNUSED(range);
    Q_UNUSED(element);
    DUChainReadLocker lock;
    QString attributeName = attribute;
    QString attributeNsPrefix;
    TopDUContext * tc = 0;
    QMap<QString, CompletionItem::Ptr > items;
    
    if (document->mimeType() != "application/xsd") {
        return items.values();
    }

    tc = DUChain::self()->chainForDocument(document->url());
    if (!tc) return items.values();

    if (attributeName.contains(":")) {
        attributeNsPrefix = attributeName.split(':')[0];
        attributeName = attributeName.split(':')[1];
    }

    //Schema Types
    if (attributeName == "type" || attributeName == "base") {
        IndexedString nsUse = IndexedString(findNamespaceUse(tc));
        QVector<KDevelop::Declaration *> decs = tc->localDeclarations();
        foreach(KDevelop::Declaration * dec, decs) {
            if (dec->kind() == KDevelop::Declaration::NamespaceAlias) {
                KDevelop::NamespaceAliasDeclaration *nsdec = dynamic_cast<KDevelop::NamespaceAliasDeclaration *>(dec);
                if (nsdec) {
                    QString ns = nsdec->importIdentifier().toString();
                    SchemaMutexLocker lock;
                    SchemaPtr schema;
                    if (ns == findNamespaceUse(tc))
                        schema = SchemaController::self()->schemaForDocument(document->url());
                    else
                        schema = SchemaController::self()->schemaForDocument(findImportForNamespace(tc, ns));
                    if (!schema) {
                        debug() << "schema not found" << ns;
                        continue;
                    }
                    QList<SchemaNodePtr> types;
                    types.append(schema->simpleTypes().values());
                    types.append(schema->complexTypes().values());
                    foreach(SchemaNodePtr n, types) {
                        QString nsPrefix = findNamespaceForPrefix(tc, n->ns().str());
                        if (n->ns() == nsUse) {
                            items.insert(n->name().str(),
                                         CompletionItem::Ptr( new XsdCompletionItem(n->name().str(),
                                                              0,
                                                              CompletionItem::Enum,
                                                              n->nodeType())));
                        } else {
                            items.insert(QString("%1:%2").arg(nsPrefix,n->name().str()),
                                         CompletionItem::Ptr(new XsdCompletionItem(QString("%1:%2").arg(nsPrefix,n->name().str()),
                                                             0,
                                                             CompletionItem::Enum,
                                                             n->nodeType())));
                        }
                    }
                }
            }
        }
    }

    if (attributeName == "ref") {
        IndexedString nsUse = IndexedString(findNamespaceUse(tc));
        QVector<KDevelop::Declaration *> decs = tc->localDeclarations();
        foreach(KDevelop::Declaration * dec, decs) {
            if (dec->kind() == KDevelop::Declaration::NamespaceAlias) {
                KDevelop::NamespaceAliasDeclaration *nsdec = dynamic_cast<KDevelop::NamespaceAliasDeclaration *>(dec);
                if (nsdec) {
                    QString ns = nsdec->importIdentifier().toString();
                    SchemaMutexLocker lock;
                    SchemaPtr schema;
                    if (ns == findNamespaceUse(tc))
                        schema = SchemaController::self()->schemaForDocument(document->url());
                    else
                        schema = SchemaController::self()->schemaForDocument(findImportForNamespace(tc, ns));
                    if (!schema) {
                        debug() << "schema not found" << ns;
                        continue;
                    }
                    QList<SchemaNodePtr> types;
                    if(element.contains("group"))
                        types.append(schema->groups().values());
                    else if(element.contains("element"))
                        types.append(schema->elements().values());
                    else if(element.contains("attribute"))
                        types.append(schema->attributes().values());
                    else if(element.contains("attributeGroup"))
                        types.append(schema->attributeGroups().values());
                    foreach(SchemaNodePtr n, types) {
                        QString nsPrefix = findNamespaceForPrefix(tc, n->ns().str());
                        if (n->ns() == nsUse) {
                            items.insert(n->name().str(),
                                         CompletionItem::Ptr( new XsdCompletionItem(n->name().str(),
                                                              0,
                                                              CompletionItem::Enum,
                                                              n->nodeType())));
                        } else {
                            items.insert(QString("%1:%2").arg(nsPrefix,n->name().str()),
                                         CompletionItem::Ptr(new XsdCompletionItem(QString("%1:%2").arg(nsPrefix,n->name().str()),
                                                             0,
                                                             CompletionItem::Enum,
                                                             n->nodeType())));
                        }
                    }
                }
            }
        }
    }

    return items.values();
}

