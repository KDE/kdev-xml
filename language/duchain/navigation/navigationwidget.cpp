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

#include "navigationwidget.h"
#include <language/duchain/declaration.h>
#include <language/duchain/duchainlock.h>
#include <language/duchain/duchain.h>
#include <elementdeclaration.h>
#include <KLocalizedString>
#include <language/duchain/navigation/abstractdeclarationnavigationcontext.h>
#include <language/duchain/namespacealiasdeclaration.h>
#include <language/duchain/aliasdeclaration.h>

using namespace Xml;
using namespace KDevelop;

class DeclarationNavigationContext : public AbstractNavigationContext {
public:
    DeclarationNavigationContext(KDevelop::DeclarationPointer declaration, KDevelop::TopDUContextPointer topContext) : 
    AbstractNavigationContext(topContext, 0) {
        m_declaration = declaration;
        m_topContext = topContext;
    }

    virtual QString name() const {
        KDevelop::DUChainReadLocker lock(KDevelop::DUChain::lock());
        ElementDeclaration *element = dynamic_cast<ElementDeclaration *>(m_declaration.data());
        if(!element) {
            return m_declaration->identifier().toString();
        }
        return element->name().str();
    }

    virtual QString html(bool shorten = false) {
        clear();

        KDevelop::DUChainReadLocker lock(KDevelop::DUChain::lock());

        //TODO a propper unresolved identifier
        if(m_declaration->identifier().isEmpty()) {
            modifyHtml() += i18n("Unable to resolve");
            return currentHtml();
        }

        if(m_declaration->kind() == KDevelop::Declaration::Alias) {
            KDevelop::AliasDeclaration *dec = dynamic_cast<KDevelop::AliasDeclaration *>(m_declaration.data());
            if(dec)
                m_declaration = KDevelop::DeclarationPointer(dec->aliasedDeclaration().data());
        }

        if(m_declaration->kind() == KDevelop::Declaration::Namespace) {
            modifyHtml() += QString("<b>%1:</b> %2").arg(i18n("Namespace"), m_declaration->identifier().toString());
            return currentHtml();
        }

        if(m_declaration->kind() == KDevelop::Declaration::NamespaceAlias) {
            KDevelop::NamespaceAliasDeclaration *dec = dynamic_cast<KDevelop::NamespaceAliasDeclaration *>(m_declaration.data());
            if(!dec) {
                modifyHtml() += i18n("Unable to resolve");
                return currentHtml();
            }
            QString ns = dec->importIdentifier().toString();
            modifyHtml() += QString("<b>%1:</b> %2").arg(i18n("Namespace"), ns);
            return currentHtml();
        }

        if(m_declaration->kind() == KDevelop::Declaration::Import) {
            QString file = m_declaration->identifier().toString();
            modifyHtml() +=QString("<b>%1:</b> %2").arg(i18n("Import"), file);
            //makeLink(file, m_declaration, NavigationAction::JumpToSource);
            return currentHtml();
        }

        ElementDeclaration *element = dynamic_cast<ElementDeclaration *>(m_declaration.data());
        bool closeTag = false; //Used for make links at end
        if(element && element->elementType() == ElementDeclarationData::CloseTag) {
            closeTag = true;
            element = dynamic_cast<ElementDeclaration *>(element->context()->owner());
        }

        if(!element) {
            modifyHtml() += i18n("Unable to resolve");
            return currentHtml();
        }

        QString name = element->prettyName();
        modifyHtml() += i18n("Path:") + "&nbsp;";
        KDevelop::DUContext *c = element->context();
        QList<ElementDeclaration *> decl;
        while(c && c->owner()) {
            ElementDeclaration *e = dynamic_cast<ElementDeclaration *>(c->owner());
            if(e) decl << e;
            c = c->owner()->context();
        }
        modifyHtml() += QString("/");
        for(int i = decl.size() -1; i > -1; --i) {
            makeLink(decl[i]->prettyName(), KDevelop::DeclarationPointer(decl[i]), NavigationAction::JumpToSource);
            modifyHtml() += QString("/");
        }
        if(closeTag)
            makeLink(name, KDevelop::DeclarationPointer(element), NavigationAction::JumpToSource);
        else
            modifyHtml() += nameHighlight(name);
        modifyHtml() += "<br/>";

        QList<KDevelop::Declaration *> declarations = element->context()->topContext()->findDeclarations(element->identifier());
        ElementDeclaration * typeDec = 0;
        foreach(KDevelop::Declaration * dec, declarations) {
            if(dec->kind() == KDevelop::Declaration::Type) {
                typeDec = dynamic_cast<ElementDeclaration *>(dec);
                if(typeDec)
                    break;
            }
        }
        if(typeDec) {
            if(!typeDec->closeTagRequired())
                modifyHtml() += i18n("Close tag: optional") + "<br/>";
            if(!typeDec->contentType().isEmpty())
                modifyHtml() += i18n("Content type: ") + Qt::escape(typeDec->contentType().str()) + "<br/>";
            if(!typeDec->content().isEmpty())
                modifyHtml() += i18n("Content: ") + Qt::escape(typeDec->content().str()) + "<br/>";
            if(typeDec->attributesSize() > 0) {
                modifyHtml() += propertyHighlight(i18n("Available Attributes:&nbsp;"));
                for(int i = 0; i < typeDec->attributesSize(); i++) {
                    if(i < typeDec->attributesSize() - 1)
                        modifyHtml() += typeDec->attributes()[i].str() + ", ";
                    else
                        modifyHtml() += typeDec->attributes()[i].str() + " ";
                }
                modifyHtml() += "<br/>";
            }
            if(typeDec->internalContext()) {
                QVector<KDevelop::Declaration *> declarations = typeDec->internalContext()->localDeclarations();
                    if(declarations.size() > 0) {
                    modifyHtml() += propertyHighlight(i18n("Available Children:&nbsp;"));
                    for(int i = 0; i < declarations.size(); i++) {
                        if(declarations[i] && declarations[i]->kind() == KDevelop::Declaration::Instance) {
                            if(i < declarations.size() - 1)
                                modifyHtml() += declarations[i]->identifier().toString() + ", ";
                            else
                                modifyHtml() += declarations[i]->identifier().toString() + " ";
                        }
                    }
                }
            }
        }
        modifyHtml() += " ";
        return currentHtml();
    }

protected:
    KDevelop::DeclarationPointer m_declaration;
    KDevelop::TopDUContextPointer m_topContext;
};

NavigationWidget::NavigationWidget(KDevelop::DeclarationPointer declaration, KDevelop::TopDUContextPointer topContext)
{
    m_topContext = topContext;
    m_declaration = declaration;
    initBrowser(50);
    //The first context is registered so it is kept alive by the shared-pointer mechanism
    m_startContext = NavigationContextPointer(new DeclarationNavigationContext(declaration, topContext));
    setContext(m_startContext);
}
