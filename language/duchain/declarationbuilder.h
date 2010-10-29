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

#ifndef XML_DECLARATIONBUILDER_H
#define XML_DECLARATIONBUILDER_H

#include "duchainexport.h"

#include "contextbuilder.h"

#include <language/duchain/builders/abstractdeclarationbuilder.h>
#include "elementdeclaration.h"

namespace KDvelop
{
class Declaration;
class QualifiedIdentifier;
}

namespace Xml
{
class ParseSession;
class EditorIntegrator;
class ElementDeclaration;

typedef KDevelop::AbstractDeclarationBuilder<AstNode, ElementAst, ContextBuilder> DeclarationBuilderBase;

/** Builder for XML SGML and DTD
 *  Sgml is case insensitive, therefor all Indentifiers is in lower case, but ElementDeclaration has
 *  a method 'name' and 'prettyName' which is case sensitive.
 *  @see ElementDeclaration
 *  @see XsdDeclarationBuilder
 */
class KDEVSGMLDUCHAIN_EXPORT DeclarationBuilder : public DeclarationBuilderBase
{
public:
    DeclarationBuilder(EditorIntegrator* editor);
    ~DeclarationBuilder();

    virtual void visitElementTag(ElementTagAst* node);
    virtual void visitElementCloseTag(ElementCloseTagAst* node);
    virtual void visitAttribute(AttributeAst* node);
    virtual void visitDtdDoctype(DtdDoctypeAst* node);
    virtual void visitDtdElement(DtdElementAst* node);
    virtual void visitDtdElementList(DtdElementListAst* node);
    virtual void visitDtdElementIncludeItem(DtdElementIncludeItemAst* node);
    virtual void visitDtdAttlist(DtdAttlistAst* node);
    virtual void visitDtdEntity(DtdEntityAst* node);
    virtual void visitDtdEntityInclude(DtdEntityIncludeAst* node);
    virtual void visitElementPCDATA(ElementPCDATAAst* node);
    virtual void visitElementCDATA(ElementCDATAAst* node);
    virtual void visitElementText(ElementTextAst* node);
    virtual void closeDeclaration();

protected:

    /** Creates an element instance declaration
     *  @note The chain must be unlocked
     */
    ElementDeclaration* createClassInstanceDeclaration(const QString &identifier,
            const KDevelop::RangeInRevision &range,
            ElementDeclarationData::ElementType type,
            const QString &nsPrefix = QString());

    /** Creates an alias declaration
     *  @note The chain must be unlocked
     */
    KDevelop::Declaration * createAliasDeclaration(const QString &identifier,
            const KDevelop::RangeInRevision &range,
            KDevelop::Declaration *alias);

    /** Creates an import declaration
     *  @note The chain must be unlocked
     */
    KDevelop::Declaration *createImportDeclaration(const QString &identifier,
            const KDevelop::RangeInRevision &range,
            const KUrl &url);
    QHash<QString, QString> m_dtdElementExclude;

    /** Finds a namespace
     *  The QID is used in alias declarations etc.
     *  @note The chain must be locked
     */
    KDevelop::Declaration *findNamespaceDeclaration(KDevelop::TopDUContext *context, const QString &ns);

    /** Finds a namespace alias
     *  The QID is used in alias declarations etc.
     *  @note The chain must be locked
     */
    KDevelop::Declaration *findNamespaceAliasDeclaration(KDevelop::TopDUContext *context, const QString &ns);
    
    /** Finds an attribute with name 'name' */
    AttributeAst * attribute(ElementTagAst *node, const QString &name) const;

    /** Keeps a list of defined elements
     *  Used in visitDtdAttlist. */
    QHash<QString, ElementDeclaration*> m_dtdElements;
    QHash<QString, KDevelop::Declaration *> m_nameSpaceAliases;
    
    /** XML files that may define a schema ie WSDL, XSD etc
     */
    bool m_hasSchema;

};

}

#endif // XML_DECLARATIONBUILDER_H
