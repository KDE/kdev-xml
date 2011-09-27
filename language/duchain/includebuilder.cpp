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

#include "includebuilder.h"

#include "editorintegrator.h"
#include "parsesession.h"
#include "../language_debug.h"

using namespace Xml;

IncludeBuilder::IncludeBuilder(EditorIntegrator* editor): AbstractBuilder()
{
    m_editor = editor;
    m_hasSchema = false;
    if(!m_editor->mime().isNull())
        m_hasSchema = m_editor->mime()->is("application/xsd");
}

QMap< AstNode*, IncludeBuilder::IncludeIdentifier> IncludeBuilder::includes()
{
    return m_includes;
}

void IncludeBuilder::visitDtdDoctype(DtdDoctypeAst* node)
{
    IncludeIdentifier id;
    if (node->publicId) {
        id.publicId = KDevelop::IndexedString(nodeText(node->publicId));
    }
    if (node->systemId) {
        id.systemId = KDevelop::IndexedString(nodeText(node->systemId));
    }
    if (node->name) {
        id.doctype = KDevelop::IndexedString(nodeText(node->name));
    }
    if (!id.doctype.isEmpty() || !id.systemId.isEmpty() || !id.publicId.isEmpty()) {
        m_includes.insert(node, id);
    } else {
        reportProblem(KDevelop::ProblemData::Error, node, QString("Syntax error"));
    }
    DefaultVisitor::visitDtdDoctype(node);
}

void IncludeBuilder::visitDtdEntity(DtdEntityAst* node)
{
    IncludeIdentifier id;
    if (node->publicId) {
        id.publicId = KDevelop::IndexedString(nodeText(node->publicId));
    }
    if (node->systemId) {
        id.systemId = KDevelop::IndexedString(nodeText(node->systemId));
        id.uri = id.systemId;
    }
    if (node->name && (!id.publicId.isEmpty() || !id.systemId.isEmpty())) {
        m_entities.insert(nodeText(node->name), id);
    }
    DefaultVisitor::visitDtdEntity(node);
}

void IncludeBuilder::visitDtdEntityInclude(DtdEntityIncludeAst* node)
{
    QString entity;
    if (node->name) {
        entity = nodeText(node->name);
    }
    if (!entity.isEmpty() && m_entities.contains(entity)) {
        m_includes.insert(node, m_entities.value(entity));
    } else {
        reportProblem(KDevelop::ProblemData::Warning, node, QString("Unable resolve entity: %1").arg(entity));
    }
    DefaultVisitor::visitDtdEntityInclude(node);
}

void IncludeBuilder::visitElementTag(ElementTagAst* node)
{
    if(m_hasSchema) {
        debug();
        /*
        if(node && node->name && nodeText(node->name) == "import") {
            IncludeIdentifier id;
            id.uri = KDevelop::IndexedString(nodeText(attribute(node,"namespace")));
            id.systemId = KDevelop::IndexedString(nodeText(attribute(node,"schemaLocation")));
            if(!id.isNull())
                m_includes.insert(node, id);
        }
        */
    }
    Xml::DefaultVisitor::visitElementTag(node);
}


void IncludeBuilder::visitAttribute(AttributeAst* node)
{
    /*TODO need some namespace processing as schemaLocation and noNamespaceSchemaLocation
      must be of schema instance. http://www.w3.org/2001/XMLSchema-instance
      NOTE we map systemid and uri as the catalog will try both, in that order.
     */
    Xml::DefaultVisitor::visitAttribute(node);
    IncludeIdentifier id;
    if (node->ns && node->value && nodeText(node->ns) == "xmlns") {
        id.uri = KDevelop::IndexedString(nodeText(node->value));
    } else if (node->name && node->value && nodeText(node->name) == "xmlns") {
        id.uri = KDevelop::IndexedString(nodeText(node->value));
    } else if (node->name && node->value && nodeText(node->name) == "schemaLocation") {
        QStringList values = nodeText(node->value).split(QRegExp("\\s+"));
        for (int i = 0; i < values.length() && values.length()%2==0; i+=2) {
            id.systemId = KDevelop::IndexedString(values[i+1]);
            id.uri = KDevelop::IndexedString(values[i]);
        }
    } else if (node->name && node->value && nodeText(node->name) == "noNamespaceSchemaLocation") {
        id.systemId = KDevelop::IndexedString(nodeText(node->value));
    }
    if(!id.isNull())
        m_includes.insert(node, id);
}

void IncludeBuilder::build(const KDevelop::IndexedString& document, AstNode* ast)
{
    m_document = document;
    visitNode(ast);
}


AttributeAst * IncludeBuilder::findAttribute(ElementTagAst* node, const QString &name) const
{
    if(!node || !node->attributesSequence)
        return 0;

    for(int i = 0; i < node->attributesSequence->count(); i++) {
        AttributeAst *att = node->attributesSequence->at(i)->element;
        if(att && att->name) {
            QString attName = nodeText(att->name);
            if(attName == name)
                return att;
        }
    }
    return 0;
}

QString IncludeBuilder::tokenText(qint64 begin, qint64 end) const
{
    EditorIntegrator *e = static_cast<EditorIntegrator *>(editor());
    return e->parseSession()->contents().mid(begin, end - begin + 1);
}

QString IncludeBuilder::nodeText(AstNode *node) const
{
    if (!node) return QString();
    EditorIntegrator *e = static_cast<EditorIntegrator *>(editor());
    return tokenText(e->parseSession()->tokenStream()->at(node->startToken).begin, e->parseSession()->tokenStream()->at(node->endToken).end);
}

void Xml::IncludeBuilder::reportProblem(KDevelop::ProblemData::Severity, AstNode* ast, const QString& message)
{
    KDevelop::Problem *p = new KDevelop::Problem();
    p->setSource(KDevelop::ProblemData::Parser);
    p->setSeverity(KDevelop::ProblemData::Error);
    p->setDescription(message);
    KDevelop::RangeInRevision range = editor()->findRange(ast);
    p->setFinalLocation(KDevelop::DocumentRange(m_document, KTextEditor::Range(range.start.line, range.start.column, range.end.line, range.end.column)));
    m_problems << KDevelop::ProblemPointer(p);
}

