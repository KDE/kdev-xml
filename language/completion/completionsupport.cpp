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

#include "completionsupport.h"

#include "elementdeclaration.h"
#include "completionvisitor.h"

#include "../language_debug.h"

#include <schema/schema.h>

#include <QtGui/QIcon>

#include <KDE/KTextEditor/View>
#include <KDE/KTextEditor/Document>
#include <KDE/KMimeType>

#include <language/duchain/topducontext.h>
#include <language/duchain/duchainlock.h>
#include <language/duchain/duchain.h>
#include <language/duchain/declaration.h>
#include <language/duchain/classdeclaration.h>
#include <language/duchain/namespacealiasdeclaration.h>

#include <interfaces/icore.h>
#include <interfaces/isourceformattercontroller.h>
#include <interfaces/isourceformatter.h>

using namespace Xml;
using namespace KDevelop;
using namespace KTextEditor;

//Chain must be locked before calling
QString CompletionSupport::findPrefixForNamespace(KDevelop::TopDUContext *topContext, const QString &nsPrefix) const
{
    if (!topContext) return QString();
    QList<KDevelop::Declaration *> decs = topContext->findDeclarations(KDevelop::Identifier(nsPrefix));
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

QString CompletionSupport::findNamespaceUse(TopDUContext* topContext) const
{
    if (!topContext) return QString();
    QList<KDevelop::Declaration *> decs = topContext->findDeclarations(globalImportIdentifier());
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

QString CompletionSupport::findNamespaceForPrefix(TopDUContext* topContext, const QString& ns) const
{
    if (!topContext) return QString();
    QVector<KDevelop::Declaration *> decs = topContext->localDeclarations();
    foreach(KDevelop::Declaration * dec, decs) {
        if (dec->kind() == KDevelop::Declaration::NamespaceAlias) {
            KDevelop::NamespaceAliasDeclaration *nsdec = dynamic_cast<KDevelop::NamespaceAliasDeclaration *>(dec);
            if (nsdec && nsdec->importIdentifier().toString() == ns)
                return nsdec->identifier().toString();
        }
    }
    return QString();
}

KDevelop::IndexedString CompletionSupport::findImportForNamespace(TopDUContext* tc, const QString& ns) const
{               
    foreach(KDevelop::DUContext::Import import, tc->importedParentContexts()) {
        QVector<KDevelop::Declaration *> decs = import.indexedContext().context()->topContext()->localDeclarations();
        foreach(KDevelop::Declaration * dec, decs) {
            if (dec->kind() == KDevelop::Declaration::Namespace) {
                if (dec->identifier().toString() == ns)
                    return import.indexedContext().context()->url();
            }
        }
    }
    return KDevelop::IndexedString();
}

