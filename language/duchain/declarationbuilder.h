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
 *  @see XsdDeclarationBuilder
 */
class KDEVSGMLDUCHAIN_EXPORT DeclarationBuilder : public DeclarationBuilderBase
{
public:
    DeclarationBuilder(KDevelop::EditorIntegrator* editor);
    ~DeclarationBuilder();

    virtual void visitElementTag(ElementTagAst* node);
    virtual void visitAttribute(AttributeAst* node);
    virtual void visitDtdDoctype(DtdDoctypeAst* node);
    virtual void visitDtdElement(DtdElementAst* node);
    virtual void visitDtdElementList(DtdElementListAst* node);
    virtual void visitDtdElementIncludeItem(DtdElementIncludeItemAst* node);
    virtual void visitDtdEntity(DtdEntityAst* node);
    virtual void visitDtdEntityInclude(DtdEntityIncludeAst* node);
    virtual void visitElementPHP(ElementPHPAst* node);
    virtual void visitElementPCDATA(ElementPCDATAAst* node);
    virtual void visitElementCDATA(ElementCDATAAst* node);
    virtual void visitElementText(ElementTextAst* node);
    virtual void visitElementProcessing(ElementProcessingAst* node);
    virtual void visitElementSource(ElementSourceAst* node);
    virtual void closeDeclaration();

protected:
    virtual KDevelop::QualifiedIdentifier identifierForNode( ElementTagAst* node );
    ElementDeclaration* createClassInstanceDeclaration(const QString &identifier,
                                        const KDevelop::SimpleRange &range,
                                        const QString &name = QString(),
                                        const QString &nameSpacePrefix = QString());

    KDevelop::Declaration *createImportDeclaration(const QString &identifier,
                                                   const KDevelop::SimpleRange &range,
                                                   const KUrl &url);
    QHash<QString, QString> m_dtdElementExclude;
};

}

#endif // XML_DECLARATIONBUILDER_H
