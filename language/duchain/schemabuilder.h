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


#ifndef SCHEMABUILDER_H
#define SCHEMABUILDER_H

#include <QMap>
#include <language/duchain/indexedstring.h>

#include "duchainexport.h"
#include "sgmldebugvisitor.h"
#include "abstractbuilder.h"
#include "schema/schema.h"

namespace Xml {
class EditorIntegrator;

/** Builds a XSD Schema using the Ast tree
 *  Using the Ast tree as a DOM tree build the schema for the document.
 */
class KDEVSGMLDUCHAIN_EXPORT SchemaBuilder : public AbstractBuilder
{
public:
    SchemaBuilder(EditorIntegrator* editor);
    virtual ~SchemaBuilder();
    void build(const KDevelop::IndexedString &document, AstNode* ast);

    EditorIntegrator* editor() const {
        return m_editor;
    }

protected:
    virtual void visitElementTag(ElementTagAst* node);
    virtual void visitElementCloseTag(ElementCloseTagAst* node);

private:
    struct SchemaBuilderPrivate;
    struct SchemaBuilderPrivate *d;
    QString nodeText(AstNode *node) const;
    QString tokenText(qint64 begin, qint64 end) const;
    EditorIntegrator* m_editor;
    virtual void reportProblem(KDevelop::ProblemData::Severity , AstNode *ast, const QString& message);
    KDevelop::IndexedString m_document;

    /** Finds an attribute with name 'name' */
    AttributeAst * findAttribute(ElementTagAst *node, const QString &name) const;

    /** Finds a namespace for a namespace prefix in the DUChain */
    QString findNamespaceForPrefix(const QString &prefix) const;
    
    /** Finds a schema node for a prefixed name ie xs:group */
    SchemaNodePtr nodeForPrefixedName(const QString &prefixedName) const;
    
    int m_passCount;
};

}

#endif // SCHEMABUILDER_H
