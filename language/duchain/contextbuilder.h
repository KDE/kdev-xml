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

#ifndef XML_CONTEXTBUILDER_H
#define XML_CONTEXTBUILDER_H

#include "duchainexport.h"

#include "sgmldebugvisitor.h"

#include <language/duchain/builders/abstractcontextbuilder.h>
#include "abstractbuilder.h"

namespace Xml
{
class EditorIntegrator;
class ParseSession;

typedef KDevelop::AbstractContextBuilder<AstNode, IdentifierAst> ContextBuilderBase;

/** Builder for XML SGML and DTD
 *  Sgml is case insensitive, therefor all Indentifiers is in lower case, but ElementDeclaration has
 *  a method 'name' and 'prettyName' which is case sensitive.
 *  @see XsdContextBuilder
 *  @see ElementDeclaration
 */
class KDEVSGMLDUCHAIN_EXPORT ContextBuilder: public ContextBuilderBase, public AbstractBuilder
{
public:
    ContextBuilder();

    virtual ~ContextBuilder();

    virtual KDevelop::ReferencedTopDUContext build(const KDevelop::IndexedString& url, AstNode* node, KDevelop::ReferencedTopDUContext updateContext = KDevelop::ReferencedTopDUContext());

protected:
    EditorIntegrator* editor() const;
    void setEditor(EditorIntegrator * editor);

    virtual void startVisiting(AstNode* node);
    virtual void visitElementTag(ElementTagAst* node);
    virtual void visitAttribute(AttributeAst* node);
    virtual void visitDtdCondition(DtdConditionAst* node);
    virtual void visitDtdDoctype(DtdDoctypeAst* node);
    virtual void visitDtdElement(DtdElementAst* node);
    virtual void visitDtdEntityInclude(DtdEntityIncludeAst* node);
    virtual KDevelop::DUContext* contextFromNode(AstNode* node);
    virtual KDevelop::RangeInRevision editorFindRange(AstNode* fromNode, AstNode* toNode);
    virtual KDevelop::QualifiedIdentifier identifierForNode(IdentifierAst* node);
    virtual void setContextOnNode(AstNode* node, KDevelop::DUContext* context);
    virtual KDevelop::TopDUContext* newTopContext(const KDevelop::RangeInRevision& range, KDevelop::ParsingEnvironmentFile* file = 0);
    virtual KDevelop::CursorInRevision findElementChildrenReach(ElementTagAst* node);

    KDevelop::RangeInRevision nodeRange(AstNode *node) const;
    QString nodeText(AstNode *node) const;
    QString tokenText(qint64 begin, qint64 end) const;
    
    QString tagName(const ElementTagAst *ast) const;
    QString tagName(const ElementCloseTagAst *ast) const;

    /** Creates a namespaced identifier
     *  The identifier is in lower case.<br/>
     *  ElementDeclaration has a prettyName, name and ns member fuctions which returns
     *  case sensitive names.
     */
    KDevelop::QualifiedIdentifier namespacedIdentifier(const ElementTagAst *ast) const;

    /** Creates a namespaced identifier
     *  The identifier is in lower case.<br/>
     *  ElementDeclaration has a prettyName, name and ns member fuctions which returns
     *  case sensitive names.
     */
    KDevelop::QualifiedIdentifier namespacedIdentifier(const ElementCloseTagAst *ast) const;

    /** Creates a namespaced identifier
     *  The identifier is in lower case.<br/>
     *  ElementDeclaration has a prettyName, name and ns member fuctions which returns
     *  case sensitive names.
     */
    KDevelop::QualifiedIdentifier namespacedIdentifier(const AttributeAst *ast) const;

    /** The current elements context to build
     *  &lt;!ELEMENT (element1 | element2 | element3 ) ...context... &gt;<br/>
     *  This must be set to each of the element*'s and visitDtdElement
     *  must be called for each of them.
     */
    void setDtdElementId(const KDevelop::QualifiedIdentifier & id) {
        m_dtdElementId = id;
    }

    KDevelop::QualifiedIdentifier m_dtdElementId;

    virtual void reportProblem(KDevelop::ProblemData::Severity, AstNode* ast, const QString& message);

    bool m_mapAst;
    EditorIntegrator* m_editor;
};
}

#endif
