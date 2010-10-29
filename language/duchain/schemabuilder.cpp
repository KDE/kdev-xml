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

#include "schemabuilder.h"

#include "editorintegrator.h"
#include "parsesession.h"
#include <language/duchain/duchainlock.h>
#include <language/duchain/duchain.h>
#include <language/duchain/declaration.h>
#include <language/duchain/namespacealiasdeclaration.h>

#include "../language_debug.h"
#include <xmlcatalog/cataloghelper.h>

using namespace Xml;

typedef struct SchemaBuilder::SchemaBuilderPrivate {
public:
    SchemaBuilderPrivate() {}
    ~SchemaBuilderPrivate() {}
    SchemaPtr schema;
    QStack<SchemaNodePtr> stack;
} SchemaBuilderPrivate;


SchemaBuilder::SchemaBuilder(EditorIntegrator* editor): AbstractBuilder()
{
    m_editor = editor;
    d = new SchemaBuilderPrivate;
    m_passCount = 0;
}

SchemaBuilder::~SchemaBuilder()
{
    if (d) delete d;
}

void visitNodePtr(const SchemaNodePtr &n, QString depth) {
    //Prevent recurtion
    if (depth.size() > 10) return;
    foreach(const SchemaNodePtr &e, n->elements().values()) {
        debug() << "e" << depth + e->name().str();
        visitNodePtr(e, depth + "-");
    }
    foreach(const SchemaNodePtr &e, n->attributes().values()) {
        debug() << "a" << depth + e->name().str();
        visitNodePtr(e, depth + "-");
    }
    foreach(const SchemaNodePtr &e, n->enums().values()) {
        const SchemaEnum *en = dynamic_cast<const SchemaEnum *>(e.data());
        if (en)
            debug() << "n" << depth + en->value().str();
    }
}

void SchemaBuilder::build(const KDevelop::IndexedString& document, AstNode* ast)
{
    debug();
    m_document = document;

    d->stack.clear();

    //First pass
    visitNode(ast);

    m_passCount++;
    d->stack.clear();
    d->stack.push(SchemaNodePtr::dynamicCast<Schema>(d->schema));
    d->stack.top();

    //Second pass
    visitNode(ast);

    /*
    SchemaMutexLocker lock;
    Schema *s = dynamic_cast<Schema *>(d->schema.data());
    if(s) {
        foreach(const SchemaNodePtr &n, s->nodes()) {
            debug() << n->name().str();
            visitNodePtr(n , "-");
        }
    }
    */
}

void SchemaBuilder::visitElementTag(ElementTagAst* node)
{
    if (!node || !node->name) return;

    //Pop the stack if we have have a valid ptr and after visiting the children.
    SchemaNodePtr ptr;

    QString name = nodeText(node->name);

    if (name == "schema" && m_passCount < 1) {
        AttributeAst *ns = findAttribute(node, "targetNamespace");
        AttributeAst *elementFormDefault = findAttribute(node, "elementFormDefault");
        AttributeAst *attributeFormDefault = findAttribute(node, "attributeFormDefault");
        if (ns) {
            d->schema = new Schema(KDevelop::IndexedString(nodeText(ns->value)), m_document);
            debug() << nodeText(ns->value);
            d->stack.push(SchemaNodePtr::dynamicCast<Schema>(d->schema));
            if (elementFormDefault && elementFormDefault->value && nodeText(elementFormDefault->value) == "qualified")
                d->schema->setQualifiedElements(true);
            if (attributeFormDefault && attributeFormDefault->value && nodeText(attributeFormDefault->value) == "qualified")
                d->schema->setQualifiedAttributes(true);
            SchemaMutexLocker lock;
            SchemaController::self()->insertSchema(d->schema);
        }
    }

    if (d->schema && !d->stack.isEmpty()) {
        
        if (name == "group") {
            AttributeAst *ref = findAttribute(node, "ref");
            if (ref) {
                //Dont push
                QString refStr = nodeText(ref->value);
                SchemaNodePtr refPtr = nodeForPrefixedName(refStr);
                if (refPtr) {
                    SchemaMutexLocker lock;
                    d->stack.top()->add(refPtr);
                } else {
                    debug() << "Unable to find:" << refStr;
                }
            } else {
                AttributeAst *name = findAttribute(node, "name");
                if (name && name->value) {
                    SchemaQName qname(KDevelop::IndexedString(nodeText(name->value)), d->schema->ns());
                    ptr = new SchemaGroup(qname);
                    SchemaMutexLocker lock;
                    d->stack.top()->add(ptr);
                    d->stack.push(ptr);
                    d->schema->addNode(ptr);
                }
            }
        }

        else if (name == "simpleType" && !d->stack.isEmpty()) {
            AttributeAst *name = findAttribute(node, "name");
            if (name && name->value) {
                SchemaQName qname(KDevelop::IndexedString(nodeText(name->value)), d->schema->ns());
                ptr = new SchemaSimpleType(qname);
                SchemaMutexLocker lock;
                d->stack.top()->add(ptr);
                d->stack.push(ptr);
                d->schema->addNode(ptr);
            }
        }

        else if (name == "complexType" && !d->stack.isEmpty()) {
            AttributeAst *name = findAttribute(node, "name");
            if (name && name->value) {
                SchemaQName qname(KDevelop::IndexedString(nodeText(name->value)), d->schema->ns());
                ptr = new SchemaComplexType(qname);
                SchemaMutexLocker lock;
                d->stack.top()->add(ptr);
                d->stack.push(ptr);
                d->schema->addNode(ptr);
            }
        }

        else if (name == "attributeGroup" && !d->stack.isEmpty()) {
            AttributeAst *ref = findAttribute(node, "ref");
            if (ref) {
                //Dont push
                QString refStr = nodeText(ref->value);
                SchemaNodePtr refPtr = nodeForPrefixedName(refStr);
                if (refPtr) {
                    SchemaMutexLocker lock;
                    d->stack.top()->add(refPtr);
                } else {
                    debug() << "Unable to find reference:" << refStr;
                }
            } else {
                AttributeAst *name = findAttribute(node, "name");
                if (name && name->value) {
                    SchemaQName qname(KDevelop::IndexedString(nodeText(name->value)), d->schema->ns());
                    ptr = new SchemaAttributeGroup(qname);
                    SchemaMutexLocker lock;
                    d->stack.top()->add(ptr);
                    d->stack.push(ptr);
                    d->schema->addNode(ptr);
                }
            }
        }

        else if (name == "element" && !d->stack.isEmpty()) {
            AttributeAst *ref = findAttribute(node, "ref");
            if (ref) {
                //Dont push
                QString refStr = nodeText(ref->value);
                SchemaNodePtr refPtr = nodeForPrefixedName(refStr);
                if (refPtr) {
                    SchemaMutexLocker lock;
                    d->stack.top()->add(refPtr);
                } else {
                    debug() << "Unable to find reference:" << refStr;
                }
            } else {
                AttributeAst *name = findAttribute(node, "name");
                if (name && name->value) {
                    SchemaQName qname(KDevelop::IndexedString(nodeText(name->value)), d->schema->ns());
                    ptr = new SchemaElement(qname);
                    AttributeAst *type = findAttribute(node, "type");
                    if (type) {
                        QString refStr = nodeText(type->value);
                        SchemaNodePtr refPtr = nodeForPrefixedName(refStr);
                        if (refPtr)
                            ptr->add(refPtr);
                        else
                            debug() << "Unable to find:" << refStr;
                    }
                    SchemaMutexLocker lock;
                    d->stack.top()->add(ptr);
                    d->stack.push(ptr);
                    d->schema->addNode(ptr);
                }
            }
        }

        else if (name == "extension" && !d->stack.isEmpty()) {
            //Dont push
            AttributeAst *base = findAttribute(node, "base");
            if (base && base->value) {
                QString refStr = nodeText(base->value);
                SchemaNodePtr refPtr = nodeForPrefixedName(refStr);
                if (refPtr) {
                    SchemaMutexLocker lock;
                    d->stack.top()->add(refPtr);
                } else {
                    debug() << "Unable to find reference:" << refStr;
                }
            }
        }

        else if (name == "restriction" && !d->stack.isEmpty()) {
            //Dont push
            AttributeAst *base = findAttribute(node, "base");
            if (base && base->value) {
                QString refStr = nodeText(base->value);
                SchemaNodePtr refPtr = nodeForPrefixedName(refStr);
                if (refPtr) {
                    SchemaMutexLocker lock;
                    d->stack.top()->add(refPtr);
                } else {
                    debug() << "Unable to find reference:" << refStr;
                }
            }
        }

        else if (name == "documentation" && !d->stack.isEmpty()) {
            if (!d->stack.isEmpty() && node->childrenSequence && node->childrenSequence->count() > 0)
                d->stack.top()->setAnnotation(KDevelop::IndexedString(nodeText(node->childrenSequence->at(0)->element)));
        }

        else if (name == "attribute" && !d->stack.isEmpty()) {
            AttributeAst *ref = findAttribute(node, "ref");
            if (ref) {
                //Dont push
                QString refStr = nodeText(ref->value);
                SchemaNodePtr refPtr = nodeForPrefixedName(refStr);
                if (refPtr) {
                    SchemaMutexLocker lock;
                    d->stack.top()->add(ptr);
                } else {
                    debug() << "Unable to find reference:" << refStr;
                }
            } else {
                AttributeAst *name = findAttribute(node, "name");
                if (name && name->value) {
                    SchemaQName qname(KDevelop::IndexedString(nodeText(name->value)), d->schema->ns());
                    ptr = new SchemaAttribute(qname);
                    AttributeAst *type = findAttribute(node, "type");
                    if (type) {
                        QString refStr = nodeText(type->value);
                        SchemaNodePtr refPtr = nodeForPrefixedName(refStr);
                        if (refPtr)
                            ptr->add(refPtr);
                        else
                            debug() << "Unable to find reference:" << refStr;
                    }
                    SchemaMutexLocker lock;
                    d->stack.top()->add(ptr);
                    d->stack.push(ptr);
                    d->schema->addNode(ptr);
                }
            }
        }

        else if (name == "enumeration" && !d->stack.isEmpty()) {
            AttributeAst *value = findAttribute(node, "value");
            if (value && value->value) {
                QString enumValue = nodeText(value->value);
                QString name = QString("{%1-enum-%2}").arg(d->stack.top()->name().str()).arg(d->stack.top()->enums().size());
                SchemaQName qname(KDevelop::IndexedString(name), d->schema->ns());
                SchemaEnum *e = new SchemaEnum(qname);
                e->setValue(KDevelop::IndexedString(enumValue));
                ptr = e;
                SchemaMutexLocker lock;
                d->stack.top()->add(ptr);
                d->stack.push(ptr);
                d->schema->addNode(ptr);
            }
        } else if (name == "include" && !d->stack.isEmpty()) {
            debug();
            /*
            IncludeIdentifier id;
            QString document = nodeText(attribute(node,"schemaLocation"));
            KUrl url = CatalogHelper::resolve(QString(), document, QString(), QString(), KMimeType::Ptr(), m_document.toUrl());
            if (url.isValid()) {
                if(url != editor()->currentUrl().toUrl()) {
                    SchemaPtr schema = SchemaController::self()->schemaForDocument(url);
                    if (schema) {
                        d->schema->add(schema);
                    }
                }
            }
            */
        }
    }

    DefaultVisitor::visitElementTag(node);

    if (!d->stack.isEmpty() && d->stack.top() == ptr)
        d->stack.pop();
}

void SchemaBuilder::visitElementCloseTag(ElementCloseTagAst* node)
{
    DefaultVisitor::visitElementCloseTag(node);
}

QString SchemaBuilder::nodeText(AstNode *node) const
{
    return editor()->parseSession()->symbol(node);
}

QString SchemaBuilder::tokenText(qint64 begin, qint64 end) const
{
    return editor()->parseSession()->contents().mid(begin, end - begin + 1);
}

void SchemaBuilder::reportProblem(KDevelop::ProblemData::Severity , AstNode* ast, const QString& message)
{
    KDevelop::Problem *p = new KDevelop::Problem();
    p->setSource(KDevelop::ProblemData::Parser);
    p->setSeverity(KDevelop::ProblemData::Error);
    p->setDescription(message);
    KDevelop::RangeInRevision range = editor()->findRange(ast);
    p->setFinalLocation(KDevelop::DocumentRange(m_document, KTextEditor::Range(range.start.line, range.start.column, range.end.line, range.end.column)));
    m_problems << KDevelop::ProblemPointer(p);
}

AttributeAst * SchemaBuilder::findAttribute(ElementTagAst* node, const QString &name) const
{
    if (!node || !node->attributesSequence)
        return 0;

    for (int i = 0; i < node->attributesSequence->count(); i++) {
        AttributeAst *att = node->attributesSequence->at(i)->element;
        if (att && att->name) {
            QString attName = nodeText(att->name);
            if (attName == name)
                return att;
        }
    }
    return 0;
}

SchemaNodePtr SchemaBuilder::nodeForPrefixedName(const QString &prefixedName) const
{
    QString ns = d->schema->ns().str();
    QString name = prefixedName;
    if (prefixedName.contains(":")) {
        //Find the namespace in the DUChain
        ns = findNamespaceForPrefix(prefixedName.split(":").at(0));
        name = prefixedName.split(":").at(1);
    }

    SchemaMutexLocker lock;

    SchemaNodePtr node = SchemaController::self()->node(SchemaQName(KDevelop::IndexedString(name), KDevelop::IndexedString(ns)));

    return node;
}

//Find the namespace in the DUChain
QString SchemaBuilder::findNamespaceForPrefix(const QString& prefix) const
{
    KDevelop::DUChainReadLocker lock;
    KDevelop::TopDUContext * top = KDevelop::DUChain::self()->chainForDocument(m_document);
    if (!top) return QString();
    QList<KDevelop::Declaration *> decs = top->findDeclarations(KDevelop::Identifier(prefix));
    foreach(KDevelop::Declaration * dec, decs) {
        if (dec->kind() == KDevelop::Declaration::NamespaceAlias) {
            KDevelop::NamespaceAliasDeclaration *nsdec = dynamic_cast<KDevelop::NamespaceAliasDeclaration *>(dec);
            if (nsdec)
                return nsdec->importIdentifier().toString();
        }
        if (dec->kind() == KDevelop::Declaration::Namespace)
            return dec->identifier().toString();
    }
    return QString();
}
