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

#ifndef COMPLETIONVISITOR_H
#define COMPLETIONVISITOR_H

#include <editorintegrator.h>

#include "../duchain/sgmldefaultvisitor.h"
#include "../duchain/parser/editorintegrator.h"
#include "../duchain/parser/parsesession.h"
#include "../language_debug.h"

namespace Xml {

/** Visitor to determine the completion context
 *  Since the parser runs in a different thread to the qui thread
 *  the chain may have not been updated when completion is invoked.
 *  The following information is needed:
 *  <ul>
 *   <li>
 *      Context (Parent Element) the current element context we are in. This
 *      is needed to determine the allowed elements.
 *   </li>
 *      Declaration (Current Element) the current element declaration we are in. This
 *      is needed to determine the allowed attributes.
 *   </li>
 *   <li>
 *      Attribute used to complete enumerations, default values etc.
 *   </li>
 *  </ul>
 */
class CompletionVisitor : public DefaultVisitor {
public:
    enum FormatCase {CASE_LOWER, CASE_UPPER, CASE_MIXED};
    
    CompletionVisitor(Xml::EditorIntegrator *e) {
        editor = e;
        context = 0;
        element = 0;
        attribute = 0;
    }

    virtual ~CompletionVisitor() {}

    virtual void visitAttribute(AttributeAst* node) {
        if (!node || !node->name) return;
        attribute = node;
        DefaultVisitor::visitAttribute(node);
    }

    virtual void visitElementTag(ElementTagAst* node) {
        if (!node || !node->name) return;
        attribute = 0;
        element = 0;
        debug() << (int)node->endToken;
        if (editor->parseSession()->tokenStream()->at(node->endToken).kind == Parser::Token_GT
                || (node->childrenSequence && node->childrenSequence->count() > 0)) {
            contextStack.push(node);
            context = node;
        } else
            element = node;
        DefaultVisitor::visitElementTag(node);
    }

    virtual void visitElementCloseTag(ElementCloseTagAst* node) {
        if (!contextStack.empty())
            contextStack.pop();
        if (!contextStack.empty())
            context = contextStack.top();
        else
            context = 0;

        element = 0;
        attribute = 0;
        DefaultVisitor::visitElementCloseTag(node);
    }

    QStack<ElementTagAst *> contextStack;


    QString contextName() {
        if(!context || !context->name) return QString();
        return nodeText(context->name);
    }
    
    QString contextNsPrefix() {
        if(!context || !context->ns) return QString();
        return nodeText(context->ns);
    }

    QString contextPrettyName() {
        if(!context || !context->name) return QString();
        if(context->ns)
            return QString("%1:%2").arg(nodeText(context->ns), nodeText(context->name));
        return nodeText(context->name);
    }

    QString elementName() {
        if(!element || !element->name) return QString();
        return nodeText(element->name);
    }
    
    QString elementNsPrefix() {
        if(!element || !element->ns) return QString();
        return nodeText(element->ns);
    }

    QString elementPrettyName() {
        if(!element || !element->name) return QString();
        if(element->ns)
            return QString("%1:%2").arg(nodeText(element->ns), nodeText(element->name));
        return nodeText(element->name);
    }

    QString attributeName() {
        if(!attribute || !attribute->name) return QString();
        return nodeText(attribute->name);
    }

    QString attributePrettyName() {
        if(!attribute || !attribute->name) return QString();
        if(attribute->ns)
            return QString("%1:%2").arg(nodeText(attribute->ns), nodeText(attribute->name));
        return nodeText(attribute->name);
    }
    
    QString attributeNsPrefix() {
        if(!attribute || !attribute->ns) return QString();
        return nodeText(attribute->ns);
    }
    
    /** Parent context we are in. */
    ElementTagAst *context;

    /** Current element declaration we are in. */
    ElementTagAst *element;

    /** used to complete enumerations, default values etc. */
    AttributeAst* attribute;

    Xml::EditorIntegrator *editor;

    QString nodeText(AstNode *node) const
    {
        if (!node) return QString();
        if (!editor) return QString();
        return editor->parseSession()->symbol(node);
    }

    QString tokenText(qint64 begin, qint64 end) const
    {
        if (!editor) return QString();
        return editor->parseSession()->contents().mid(begin, end - begin + 1);
    }
    
    FormatCase formatCase() {
        int lower = 0;
        int upper = 0;
        QString text = contextName() + elementName();
        for(int i = 0; i < text.size(); i++) {
            if(text[i].isUpper()) {
                upper ++;
            } else {
                lower ++;
            }
        }
        if(text.size() == lower)
            return CASE_LOWER;
        if(text.size() == upper)
            return CASE_UPPER;
        return CASE_MIXED;
    }
};

}

#endif //COMPLETIONVISITOR
