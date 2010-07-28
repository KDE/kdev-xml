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

#include "dtdcompletionsupport.h"

#include "../language_debug.h"

#include <KDE/KTextEditor/Document>
#include <KDE/KMimeType>

#include <language/duchain/topducontext.h>
#include <language/duchain/duchainlock.h>
#include <language/duchain/duchain.h>
#include <language/duchain/declaration.h>

#include <elementdeclaration.h>

using namespace Xml;
using namespace KDevelop;
using namespace KTextEditor;

void walkChain(DUContext* context, QList< Declaration* >& declarations)
{
    foreach(Declaration * d, context->localDeclarations()) {
        if (!d) continue;
        declarations.append(d);
        if (d->internalContext()) {
            walkChain(d->internalContext(), declarations);
        }
    }
    foreach(DUContext::Import i, context->importedParentContexts()) {
        if (i.indexedContext().context()) {
            walkChain(i.indexedContext().context(), declarations);
        }
    }
}

QList< CompletionItem::Ptr > DtdCompletionSupport::findAll(KTextEditor::Document* document,
        const KTextEditor::Range& range) const
{
    Q_UNUSED(range);
    DUChainReadLocker lock;
    TopDUContext * tc = 0;
    QHash < QString,  CompletionItem::Ptr > items;

    tc = DUChain::self()->chainForDocument(document->url());
    if (!tc) return items.values();

    if (document->mimeType() != "text/html") {
        ///TODO: actually find the root node, this is just a guess
        foreach(DUContext::Import i, tc->importedParentContexts()) {
            if (DUContext* ctx = i.indexedContext().context()) {
                foreach(Declaration* d, ctx->localDeclarations()) {
                    if (d->kind() == Declaration::Type) {
                        ElementDeclaration *elementDec = dynamic_cast<ElementDeclaration *>(d);
                        if (!elementDec || elementDec->elementType() != ElementDeclarationData::Element) continue;
                        QString name = elementDec->name().str();
                        bool empty = elementDec->contentType().str().toUpper() == "EMPTY";
                        debug() << name;
                        items.insert(name, CompletionItem::Ptr(new CompletionItem(name, 10, CompletionItem::Element, empty)));
                        break;
                    }
                }
            }
        }
    } else {
        items.insert("html", CompletionItem::Ptr(new CompletionItem("html", 10, CompletionItem::Element, false)));
    }

    QList< Declaration* > declarations;
    walkChain(tc, declarations);

    foreach(Declaration * d, declarations) {
        if (!d) continue;
        if (d->kind() == Declaration::Type) {
            ElementDeclaration *elementDec = dynamic_cast<ElementDeclaration *>(d);
            if (!elementDec || elementDec->elementType() != ElementDeclarationData::Element) continue;
            QString name = elementDec->name().str();
            bool empty = elementDec->contentType().str().toUpper() == "EMPTY";
            if (!items.contains(name)) {
                items.insert(name, CompletionItem::Ptr(new CompletionItem(name, 0, CompletionItem::Element, empty)));
            }
        }
    }

    return items.values();
}

QList< CompletionItem::Ptr > DtdCompletionSupport::findAttributes(
    Document* document,
    const KTextEditor::Range& range,
    const QString& element,
    const QString& nsPrefix) const
{
    Q_UNUSED(nsPrefix);
    DUChainReadLocker lock;
    DUContext * ctx = 0;
    Declaration *dec = 0;
    ElementDeclaration *elementDec = 0;
    QString elementName = element;
    QList<Declaration *> decs;
    TopDUContext * tc = 0;
    QHash< QString, CompletionItem::Ptr > items;

    tc = DUChain::self()->chainForDocument(document->url());
    if (!tc) {
        debug() << "No top context";
        return items.values();
    }

    //NO? try the chain then
    if (elementName.isEmpty()) {
        ctx = tc->findContextAt(SimpleCursor(range.start()));
        if (!ctx) {
            debug() << "No context";
            return items.values();
        }

        dec = ctx->findDeclarationAt(SimpleCursor(range.start()));
        if (!dec) {
            debug() << "No declaration";
            return items.values();
        }
        //TODO prettyname
        elementName = dec->identifier().toString();
    }

    //DTD's

    decs.append(tc->findDeclarations(Identifier(elementName.toLower()), SimpleCursor::invalid(), tc));

    foreach(Declaration * d, decs) {
        if (!d || d->kind() != Declaration::Type || !d->internalContext())
            continue;
        elementDec = dynamic_cast<ElementDeclaration *>(d);
        if (!elementDec) continue;
        for (int i = 0; i < elementDec->attributesSize(); i++) {
            IndexedString str = elementDec->attributes()[i];
            items.insert(str.str(), CompletionItem::Ptr(new CompletionItem(str.str(), 0, CompletionItem::Attribute)));
        }
    }

    return items.values();
}

QList< CompletionItem::Ptr > DtdCompletionSupport::findChildElements(
    Document* document,
    const KTextEditor::Range& range,
    const QString& element,
    const QString& nsPrefix) const
{
    Q_UNUSED(nsPrefix);
    DUChainReadLocker lock;
    DUContext * ctx = 0;
    Declaration *dec = 0;
    QString elementName = element;
    QList<Declaration *> decs;
    TopDUContext * tc = 0;
    QHash<QString, CompletionItem::Ptr > items;

    tc = DUChain::self()->chainForDocument(document->url());
    if (!tc) return items.values();

    //NO? try the chain then
    if (elementName.isEmpty()) {
        ctx = tc->findContextAt(SimpleCursor(range.start()));
        if (!ctx) return items.values();

        dec = ctx->owner();
        if (!dec) return items.values();
        //TODO prettyname
        elementName = dec->identifier().toString();
    }

    decs.append(tc->findDeclarations(Identifier(elementName.toLower()), SimpleCursor::invalid(), tc));

    foreach(Declaration * dc, decs) {
        if (!dc || dc->kind() != Declaration::Type || !dc->internalContext())
            continue;
        foreach(Declaration * d, dc->internalContext()->localDeclarations()) {
            if (!d || d->kind() != Declaration::Instance) continue;
            ElementDeclaration * elementDec = dynamic_cast<ElementDeclaration *>(d);
            if (!elementDec) continue;
            QString name = elementDec->name().str();
            bool empty = false;
            Identifier id = elementDec->identifier();
            //Find its type, this would normaly be done through a use builder
            QList<Declaration *> decList = tc->findDeclarations(id, SimpleCursor::invalid(), tc);
            foreach(Declaration * dc, decList) {
                if (!dc || dc->kind() != Declaration::Type || !dc->internalContext() || dc->identifier() != id)
                continue;
                elementDec = dynamic_cast<ElementDeclaration *>(dc);
                if(!elementDec) continue;
                empty = elementDec->contentType().str().toUpper() == "EMPTY";
            }
            if (!name.startsWith("#"))
                items.insert(name, CompletionItem::Ptr(new CompletionItem(name, 10, CompletionItem::Element, empty)));
        }
    }

    return items.values();
}

QList< CompletionItem::Ptr > DtdCompletionSupport::findHeadersForDocument(Document* document) const
{
    KMimeType::Ptr mime = KMimeType::mimeType(document->mimeType());
    QList< CompletionItem::Ptr > items;

    if (mime->is("text/html")) {
        items.append(CompletionItem::Ptr(new CompletionItem("<!DOCTYPE html>", 10, CompletionItem::Header)));
        items.append(CompletionItem::Ptr(new CompletionItem("<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 Transitional//EN\" \"http://www.w3.org/TR/html4/loose.dtd\">", 0, CompletionItem::Header)));
        items.append(CompletionItem::Ptr(new CompletionItem("<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01//EN\" \"http://www.w3.org/TR/html4/strict.dtd\">", 0, CompletionItem::Header)));
        items.append(CompletionItem::Ptr(new CompletionItem("<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 Frameset//EN\" \"http://www.w3.org/TR/html4/frameset.dtd\">", 0, CompletionItem::Header)));
    }

    if (mime->is("application/docbook+xml")) {
        items.append(CompletionItem::Ptr(new CompletionItem("<!DOCTYPE book>", 10, CompletionItem::Header)));
        items.append(CompletionItem::Ptr(new CompletionItem("<!DOCTYPE book PUBLIC \"-//OASIS//DTD DocBook XML V4.5//EN\" \"http://www.oasis-open.org/docbook/xml/4.5/docbookx.dtd\">", 0, CompletionItem::Header)));
    }

    return items;
}


QList< CompletionItem::Ptr > DtdCompletionSupport::findAllEntities(Document* document, const KTextEditor::Range& range) const
{
    Q_UNUSED(range);
    DUChainReadLocker lock;
    TopDUContext * tc = 0;
    QHash < QString,  CompletionItem::Ptr > items;

    tc = DUChain::self()->chainForDocument(document->url());
    if (!tc) return items.values();

    QList< Declaration* > declarations;
    walkChain(tc, declarations);

    foreach(Declaration * d, declarations) {
        if (!d) continue;
        if (d->kind() == Declaration::Type) {
            ElementDeclaration *elementDec = dynamic_cast<ElementDeclaration *>(d);
            if (!elementDec || elementDec->elementType() != ElementDeclarationData::Entity) continue;
            QString name = elementDec->name().str();
            QString info;
            if (!elementDec->content().isEmpty()) {
                info = elementDec->content().str();
            }
            items.insert(name, CompletionItem::Ptr(new CompletionItem(name, info, 0, CompletionItem::Entity)));
        }
    }

    //Some standard entities
    items.insert( "quot", CompletionItem::Ptr(new CompletionItem( "quot", QChar( 34 ), 0, CompletionItem::Entity ) ));
    items.insert( "amp", CompletionItem::Ptr(new CompletionItem( "amp", QChar( 38 ), 0, CompletionItem::Entity ) ));
    items.insert( "apos", CompletionItem::Ptr(new CompletionItem( "apos", QChar( 39 ), 0, CompletionItem::Entity ) ));
    items.insert( "lt", CompletionItem::Ptr(new CompletionItem( "lt", QChar( 60 ), 0, CompletionItem::Entity ) ));
    items.insert( "gt", CompletionItem::Ptr(new CompletionItem( "gt", QChar( 62 ), 0, CompletionItem::Entity ) ));
    items.insert( "nbsp", CompletionItem::Ptr(new CompletionItem( "nbsp", QChar( 160 ), 0, CompletionItem::Entity ) ));
    items.insert( "iexcl", CompletionItem::Ptr(new CompletionItem( "iexcl", QChar( 161 ), 0, CompletionItem::Entity ) ));
    items.insert( "cent", CompletionItem::Ptr(new CompletionItem( "cent", QChar( 162 ), 0, CompletionItem::Entity ) ));
    items.insert( "pound", CompletionItem::Ptr(new CompletionItem( "pound", QChar( 163 ), 0, CompletionItem::Entity ) ));
    items.insert( "curren", CompletionItem::Ptr(new CompletionItem( "curren", QChar( 164 ), 0, CompletionItem::Entity ) ));
    items.insert( "yen", CompletionItem::Ptr(new CompletionItem( "yen", QChar( 165 ), 0, CompletionItem::Entity ) ));
    items.insert( "brvbar", CompletionItem::Ptr(new CompletionItem( "brvbar", QChar( 166 ), 0, CompletionItem::Entity ) ));
    items.insert( "sect", CompletionItem::Ptr(new CompletionItem( "sect", QChar( 167 ), 0, CompletionItem::Entity ) ));
    items.insert( "uml", CompletionItem::Ptr(new CompletionItem( "uml", QChar( 168 ), 0, CompletionItem::Entity ) ));
    items.insert( "copy", CompletionItem::Ptr(new CompletionItem( "copy", QChar( 169 ), 0, CompletionItem::Entity ) ));
    items.insert( "ordf", CompletionItem::Ptr(new CompletionItem( "ordf", QChar( 170 ), 0, CompletionItem::Entity ) ));
    items.insert( "laquo", CompletionItem::Ptr(new CompletionItem( "laquo", QChar( 171 ), 0, CompletionItem::Entity ) ));
    items.insert( "not", CompletionItem::Ptr(new CompletionItem( "not", QChar( 172 ), 0, CompletionItem::Entity ) ));
    items.insert( "shy", CompletionItem::Ptr(new CompletionItem( "shy", QChar( 173 ), 0, CompletionItem::Entity ) ));
    items.insert( "reg", CompletionItem::Ptr(new CompletionItem( "reg", QChar( 174 ), 0, CompletionItem::Entity ) ));
    items.insert( "macr", CompletionItem::Ptr(new CompletionItem( "macr", QChar( 175 ), 0, CompletionItem::Entity ) ));
    items.insert( "deg", CompletionItem::Ptr(new CompletionItem( "deg", QChar( 176 ), 0, CompletionItem::Entity ) ));
    items.insert( "plusmn", CompletionItem::Ptr(new CompletionItem( "plusmn", QChar( 177 ), 0, CompletionItem::Entity ) ));
    items.insert( "sup2", CompletionItem::Ptr(new CompletionItem( "sup2", QChar( 178 ), 0, CompletionItem::Entity ) ));
    items.insert( "sup3", CompletionItem::Ptr(new CompletionItem( "sup3", QChar( 179 ), 0, CompletionItem::Entity ) ));
    items.insert( "acute", CompletionItem::Ptr(new CompletionItem( "acute", QChar( 180 ), 0, CompletionItem::Entity ) ));
    items.insert( "micro", CompletionItem::Ptr(new CompletionItem( "micro", QChar( 181 ), 0, CompletionItem::Entity ) ));
    items.insert( "para", CompletionItem::Ptr(new CompletionItem( "para", QChar( 182 ), 0, CompletionItem::Entity ) ));
    items.insert( "middot", CompletionItem::Ptr(new CompletionItem( "middot", QChar( 183 ), 0, CompletionItem::Entity ) ));
    items.insert( "cedil", CompletionItem::Ptr(new CompletionItem( "cedil", QChar( 184 ), 0, CompletionItem::Entity ) ));
    items.insert( "sup1", CompletionItem::Ptr(new CompletionItem( "sup1", QChar( 185 ), 0, CompletionItem::Entity ) ));
    items.insert( "ordm", CompletionItem::Ptr(new CompletionItem( "ordm", QChar( 186 ), 0, CompletionItem::Entity ) ));
    items.insert( "raquo", CompletionItem::Ptr(new CompletionItem( "raquo", QChar( 187 ), 0, CompletionItem::Entity ) ));
    items.insert( "frac14", CompletionItem::Ptr(new CompletionItem( "frac14", QChar( 188 ), 0, CompletionItem::Entity ) ));
    items.insert( "frac12", CompletionItem::Ptr(new CompletionItem( "frac12", QChar( 189 ), 0, CompletionItem::Entity ) ));
    items.insert( "frac34", CompletionItem::Ptr(new CompletionItem( "frac34", QChar( 190 ), 0, CompletionItem::Entity ) ));
    items.insert( "iquest", CompletionItem::Ptr(new CompletionItem( "iquest", QChar( 191 ), 0, CompletionItem::Entity ) ));
    items.insert( "Agrave", CompletionItem::Ptr(new CompletionItem( "Agrave", QChar( 192 ), 0, CompletionItem::Entity ) ));
    items.insert( "Aacute", CompletionItem::Ptr(new CompletionItem( "Aacute", QChar( 193 ), 0, CompletionItem::Entity ) ));
    items.insert( "Acirc", CompletionItem::Ptr(new CompletionItem( "Acirc", QChar( 194 ), 0, CompletionItem::Entity ) ));
    items.insert( "Atilde", CompletionItem::Ptr(new CompletionItem( "Atilde", QChar( 195 ), 0, CompletionItem::Entity ) ));
    items.insert( "Auml", CompletionItem::Ptr(new CompletionItem( "Auml", QChar( 196 ), 0, CompletionItem::Entity ) ));
    items.insert( "Aring", CompletionItem::Ptr(new CompletionItem( "Aring", QChar( 197 ), 0, CompletionItem::Entity ) ));
    items.insert( "AElig", CompletionItem::Ptr(new CompletionItem( "AElig", QChar( 198 ), 0, CompletionItem::Entity ) ));
    items.insert( "Ccedil", CompletionItem::Ptr(new CompletionItem( "Ccedil", QChar( 199 ), 0, CompletionItem::Entity ) ));
    items.insert( "Egrave", CompletionItem::Ptr(new CompletionItem( "Egrave", QChar( 200 ), 0, CompletionItem::Entity ) ));
    items.insert( "Eacute", CompletionItem::Ptr(new CompletionItem( "Eacute", QChar( 201 ), 0, CompletionItem::Entity ) ));
    items.insert( "Ecirc", CompletionItem::Ptr(new CompletionItem( "Ecirc", QChar( 202 ), 0, CompletionItem::Entity ) ));
    items.insert( "Euml", CompletionItem::Ptr(new CompletionItem( "Euml", QChar( 203 ), 0, CompletionItem::Entity ) ));
    items.insert( "Igrave", CompletionItem::Ptr(new CompletionItem( "Igrave", QChar( 204 ), 0, CompletionItem::Entity ) ));
    items.insert( "Iacute", CompletionItem::Ptr(new CompletionItem( "Iacute", QChar( 205 ), 0, CompletionItem::Entity ) ));
    items.insert( "Icirc", CompletionItem::Ptr(new CompletionItem( "Icirc", QChar( 206 ), 0, CompletionItem::Entity ) ));
    items.insert( "Iuml", CompletionItem::Ptr(new CompletionItem( "Iuml", QChar( 207 ), 0, CompletionItem::Entity ) ));
    items.insert( "ETH", CompletionItem::Ptr(new CompletionItem( "ETH", QChar( 208 ), 0, CompletionItem::Entity ) ));
    items.insert( "Ntilde", CompletionItem::Ptr(new CompletionItem( "Ntilde", QChar( 209 ), 0, CompletionItem::Entity ) ));
    items.insert( "Ograve", CompletionItem::Ptr(new CompletionItem( "Ograve", QChar( 210 ), 0, CompletionItem::Entity ) ));
    items.insert( "Oacute", CompletionItem::Ptr(new CompletionItem( "Oacute", QChar( 211 ), 0, CompletionItem::Entity ) ));
    items.insert( "Ocirc", CompletionItem::Ptr(new CompletionItem( "Ocirc", QChar( 212 ), 0, CompletionItem::Entity ) ));
    items.insert( "Otilde", CompletionItem::Ptr(new CompletionItem( "Otilde", QChar( 213 ), 0, CompletionItem::Entity ) ));
    items.insert( "Ouml", CompletionItem::Ptr(new CompletionItem( "Ouml", QChar( 214 ), 0, CompletionItem::Entity ) ));
    items.insert( "times", CompletionItem::Ptr(new CompletionItem( "times", QChar( 215 ), 0, CompletionItem::Entity ) ));
    items.insert( "Oslash", CompletionItem::Ptr(new CompletionItem( "Oslash", QChar( 216 ), 0, CompletionItem::Entity ) ));
    items.insert( "Ugrave", CompletionItem::Ptr(new CompletionItem( "Ugrave", QChar( 217 ), 0, CompletionItem::Entity ) ));
    items.insert( "Uacute", CompletionItem::Ptr(new CompletionItem( "Uacute", QChar( 218 ), 0, CompletionItem::Entity ) ));
    items.insert( "Ucirc", CompletionItem::Ptr(new CompletionItem( "Ucirc", QChar( 219 ), 0, CompletionItem::Entity ) ));
    items.insert( "Uuml", CompletionItem::Ptr(new CompletionItem( "Uuml", QChar( 220 ), 0, CompletionItem::Entity ) ));
    items.insert( "Yacute", CompletionItem::Ptr(new CompletionItem( "Yacute", QChar( 221 ), 0, CompletionItem::Entity ) ));
    items.insert( "THORN", CompletionItem::Ptr(new CompletionItem( "THORN", QChar( 222 ), 0, CompletionItem::Entity ) ));
    items.insert( "szlig", CompletionItem::Ptr(new CompletionItem( "szlig", QChar( 223 ), 0, CompletionItem::Entity ) ));
    items.insert( "agrave", CompletionItem::Ptr(new CompletionItem( "agrave", QChar( 224 ), 0, CompletionItem::Entity ) ));
    items.insert( "aacute", CompletionItem::Ptr(new CompletionItem( "aacute", QChar( 225 ), 0, CompletionItem::Entity ) ));
    items.insert( "acirc", CompletionItem::Ptr(new CompletionItem( "acirc", QChar( 226 ), 0, CompletionItem::Entity ) ));
    items.insert( "atilde", CompletionItem::Ptr(new CompletionItem( "atilde", QChar( 227 ), 0, CompletionItem::Entity ) ));
    items.insert( "auml", CompletionItem::Ptr(new CompletionItem( "auml", QChar( 228 ), 0, CompletionItem::Entity ) ));
    items.insert( "aring", CompletionItem::Ptr(new CompletionItem( "aring", QChar( 229 ), 0, CompletionItem::Entity ) ));
    items.insert( "aelig", CompletionItem::Ptr(new CompletionItem( "aelig", QChar( 230 ), 0, CompletionItem::Entity ) ));
    items.insert( "ccedil", CompletionItem::Ptr(new CompletionItem( "ccedil", QChar( 231 ), 0, CompletionItem::Entity ) ));
    items.insert( "egrave", CompletionItem::Ptr(new CompletionItem( "egrave", QChar( 232 ), 0, CompletionItem::Entity ) ));
    items.insert( "eacute", CompletionItem::Ptr(new CompletionItem( "eacute", QChar( 233 ), 0, CompletionItem::Entity ) ));
    items.insert( "ecirc", CompletionItem::Ptr(new CompletionItem( "ecirc", QChar( 234 ), 0, CompletionItem::Entity ) ));
    items.insert( "euml", CompletionItem::Ptr(new CompletionItem( "euml", QChar( 235 ), 0, CompletionItem::Entity ) ));
    items.insert( "igrave", CompletionItem::Ptr(new CompletionItem( "igrave", QChar( 236 ), 0, CompletionItem::Entity ) ));
    items.insert( "iacute", CompletionItem::Ptr(new CompletionItem( "iacute", QChar( 237 ), 0, CompletionItem::Entity ) ));
    items.insert( "icirc", CompletionItem::Ptr(new CompletionItem( "icirc", QChar( 238 ), 0, CompletionItem::Entity ) ));
    items.insert( "iuml", CompletionItem::Ptr(new CompletionItem( "iuml", QChar( 239 ), 0, CompletionItem::Entity ) ));
    items.insert( "eth", CompletionItem::Ptr(new CompletionItem( "eth", QChar( 240 ), 0, CompletionItem::Entity ) ));
    items.insert( "ntilde", CompletionItem::Ptr(new CompletionItem( "ntilde", QChar( 241 ), 0, CompletionItem::Entity ) ));
    items.insert( "ograve", CompletionItem::Ptr(new CompletionItem( "ograve", QChar( 242 ), 0, CompletionItem::Entity ) ));
    items.insert( "oacute", CompletionItem::Ptr(new CompletionItem( "oacute", QChar( 243 ), 0, CompletionItem::Entity ) ));
    items.insert( "ocirc", CompletionItem::Ptr(new CompletionItem( "ocirc", QChar( 244 ), 0, CompletionItem::Entity ) ));
    items.insert( "otilde", CompletionItem::Ptr(new CompletionItem( "otilde", QChar( 245 ), 0, CompletionItem::Entity ) ));
    items.insert( "ouml", CompletionItem::Ptr(new CompletionItem( "ouml", QChar( 246 ), 0, CompletionItem::Entity ) ));
    items.insert( "divide", CompletionItem::Ptr(new CompletionItem( "divide", QChar( 247 ), 0, CompletionItem::Entity ) ));
    items.insert( "oslash", CompletionItem::Ptr(new CompletionItem( "oslash", QChar( 248 ), 0, CompletionItem::Entity ) ));
    items.insert( "ugrave", CompletionItem::Ptr(new CompletionItem( "ugrave", QChar( 249 ), 0, CompletionItem::Entity ) ));
    items.insert( "uacute", CompletionItem::Ptr(new CompletionItem( "uacute", QChar( 250 ), 0, CompletionItem::Entity ) ));
    items.insert( "ucirc", CompletionItem::Ptr(new CompletionItem( "ucirc", QChar( 251 ), 0, CompletionItem::Entity ) ));
    items.insert( "uuml", CompletionItem::Ptr(new CompletionItem( "uuml", QChar( 252 ), 0, CompletionItem::Entity ) ));
    items.insert( "yacute", CompletionItem::Ptr(new CompletionItem( "yacute", QChar( 253 ), 0, CompletionItem::Entity ) ));
    items.insert( "thorn", CompletionItem::Ptr(new CompletionItem( "thorn", QChar( 254 ), 0, CompletionItem::Entity ) ));
    items.insert( "yuml", CompletionItem::Ptr(new CompletionItem( "yuml", QChar( 255 ), 0, CompletionItem::Entity ) ));
    items.insert( "OElig", CompletionItem::Ptr(new CompletionItem( "OElig", QChar( 338 ), 0, CompletionItem::Entity ) ));
    items.insert( "oelig", CompletionItem::Ptr(new CompletionItem( "oelig", QChar( 339 ), 0, CompletionItem::Entity ) ));
    items.insert( "Scaron", CompletionItem::Ptr(new CompletionItem( "Scaron", QChar( 352 ), 0, CompletionItem::Entity ) ));
    items.insert( "scaron", CompletionItem::Ptr(new CompletionItem( "scaron", QChar( 353 ), 0, CompletionItem::Entity ) ));
    items.insert( "Yuml", CompletionItem::Ptr(new CompletionItem( "Yuml", QChar( 376 ), 0, CompletionItem::Entity ) ));
    items.insert( "fnof", CompletionItem::Ptr(new CompletionItem( "fnof", QChar( 402 ), 0, CompletionItem::Entity ) ));
    items.insert( "circ", CompletionItem::Ptr(new CompletionItem( "circ", QChar( 710 ), 0, CompletionItem::Entity ) ));
    items.insert( "tilde", CompletionItem::Ptr(new CompletionItem( "tilde", QChar( 732 ), 0, CompletionItem::Entity ) ));
    items.insert( "Alpha", CompletionItem::Ptr(new CompletionItem( "Alpha", QChar( 913 ), 0, CompletionItem::Entity ) ));
    items.insert( "Beta", CompletionItem::Ptr(new CompletionItem( "Beta", QChar( 914 ), 0, CompletionItem::Entity ) ));
    items.insert( "Gamma", CompletionItem::Ptr(new CompletionItem( "Gamma", QChar( 915 ), 0, CompletionItem::Entity ) ));
    items.insert( "Delta", CompletionItem::Ptr(new CompletionItem( "Delta", QChar( 916 ), 0, CompletionItem::Entity ) ));
    items.insert( "Epsilon", CompletionItem::Ptr(new CompletionItem( "Epsilon", QChar( 917 ), 0, CompletionItem::Entity ) ));
    items.insert( "Zeta", CompletionItem::Ptr(new CompletionItem( "Zeta", QChar( 918 ), 0, CompletionItem::Entity ) ));
    items.insert( "Eta", CompletionItem::Ptr(new CompletionItem( "Eta", QChar( 919 ), 0, CompletionItem::Entity ) ));
    items.insert( "Theta", CompletionItem::Ptr(new CompletionItem( "Theta", QChar( 920 ), 0, CompletionItem::Entity ) ));
    items.insert( "Iota", CompletionItem::Ptr(new CompletionItem( "Iota", QChar( 921 ), 0, CompletionItem::Entity ) ));
    items.insert( "Kappa", CompletionItem::Ptr(new CompletionItem( "Kappa", QChar( 922 ), 0, CompletionItem::Entity ) ));
    items.insert( "Lambda", CompletionItem::Ptr(new CompletionItem( "Lambda", QChar( 923 ), 0, CompletionItem::Entity ) ));
    items.insert( "Mu", CompletionItem::Ptr(new CompletionItem( "Mu", QChar( 924 ), 0, CompletionItem::Entity ) ));
    items.insert( "Nu", CompletionItem::Ptr(new CompletionItem( "Nu", QChar( 925 ), 0, CompletionItem::Entity ) ));
    items.insert( "Xi", CompletionItem::Ptr(new CompletionItem( "Xi", QChar( 926 ), 0, CompletionItem::Entity ) ));
    items.insert( "Omicron", CompletionItem::Ptr(new CompletionItem( "Omicron", QChar( 927 ), 0, CompletionItem::Entity ) ));
    items.insert( "Pi", CompletionItem::Ptr(new CompletionItem( "Pi", QChar( 928 ), 0, CompletionItem::Entity ) ));
    items.insert( "Rho", CompletionItem::Ptr(new CompletionItem( "Rho", QChar( 929 ), 0, CompletionItem::Entity ) ));
    items.insert( "Sigma", CompletionItem::Ptr(new CompletionItem( "Sigma", QChar( 931 ), 0, CompletionItem::Entity ) ));
    items.insert( "Tau", CompletionItem::Ptr(new CompletionItem( "Tau", QChar( 932 ), 0, CompletionItem::Entity ) ));
    items.insert( "Upsilon", CompletionItem::Ptr(new CompletionItem( "Upsilon", QChar( 933 ), 0, CompletionItem::Entity ) ));
    items.insert( "Phi", CompletionItem::Ptr(new CompletionItem( "Phi", QChar( 934 ), 0, CompletionItem::Entity ) ));
    items.insert( "Chi", CompletionItem::Ptr(new CompletionItem( "Chi", QChar( 935 ), 0, CompletionItem::Entity ) ));
    items.insert( "Psi", CompletionItem::Ptr(new CompletionItem( "Psi", QChar( 936 ), 0, CompletionItem::Entity ) ));
    items.insert( "Omega", CompletionItem::Ptr(new CompletionItem( "Omega", QChar( 937 ), 0, CompletionItem::Entity ) ));
    items.insert( "alpha", CompletionItem::Ptr(new CompletionItem( "alpha", QChar( 945 ), 0, CompletionItem::Entity ) ));
    items.insert( "beta", CompletionItem::Ptr(new CompletionItem( "beta", QChar( 946 ), 0, CompletionItem::Entity ) ));
    items.insert( "gamma", CompletionItem::Ptr(new CompletionItem( "gamma", QChar( 947 ), 0, CompletionItem::Entity ) ));
    items.insert( "delta", CompletionItem::Ptr(new CompletionItem( "delta", QChar( 948 ), 0, CompletionItem::Entity ) ));
    items.insert( "epsilon", CompletionItem::Ptr(new CompletionItem( "epsilon", QChar( 949 ), 0, CompletionItem::Entity ) ));
    items.insert( "zeta", CompletionItem::Ptr(new CompletionItem( "zeta", QChar( 950 ), 0, CompletionItem::Entity ) ));
    items.insert( "eta", CompletionItem::Ptr(new CompletionItem( "eta", QChar( 951 ), 0, CompletionItem::Entity ) ));
    items.insert( "theta", CompletionItem::Ptr(new CompletionItem( "theta", QChar( 952 ), 0, CompletionItem::Entity ) ));
    items.insert( "iota", CompletionItem::Ptr(new CompletionItem( "iota", QChar( 953 ), 0, CompletionItem::Entity ) ));
    items.insert( "kappa", CompletionItem::Ptr(new CompletionItem( "kappa", QChar( 954 ), 0, CompletionItem::Entity ) ));
    items.insert( "lambda", CompletionItem::Ptr(new CompletionItem( "lambda", QChar( 955 ), 0, CompletionItem::Entity ) ));
    items.insert( "mu", CompletionItem::Ptr(new CompletionItem( "mu", QChar( 956 ), 0, CompletionItem::Entity ) ));
    items.insert( "nu", CompletionItem::Ptr(new CompletionItem( "nu", QChar( 957 ), 0, CompletionItem::Entity ) ));
    items.insert( "xi", CompletionItem::Ptr(new CompletionItem( "xi", QChar( 958 ), 0, CompletionItem::Entity ) ));
    items.insert( "omicron", CompletionItem::Ptr(new CompletionItem( "omicron", QChar( 959 ), 0, CompletionItem::Entity ) ));
    items.insert( "pi", CompletionItem::Ptr(new CompletionItem( "pi", QChar( 960 ), 0, CompletionItem::Entity ) ));
    items.insert( "rho", CompletionItem::Ptr(new CompletionItem( "rho", QChar( 961 ), 0, CompletionItem::Entity ) ));
    items.insert( "sigmaf", CompletionItem::Ptr(new CompletionItem( "sigmaf", QChar( 962 ), 0, CompletionItem::Entity ) ));
    items.insert( "sigma", CompletionItem::Ptr(new CompletionItem( "sigma", QChar( 963 ), 0, CompletionItem::Entity ) ));
    items.insert( "tau", CompletionItem::Ptr(new CompletionItem( "tau", QChar( 964 ), 0, CompletionItem::Entity ) ));
    items.insert( "upsilon", CompletionItem::Ptr(new CompletionItem( "upsilon", QChar( 965 ), 0, CompletionItem::Entity ) ));
    items.insert( "phi", CompletionItem::Ptr(new CompletionItem( "phi", QChar( 966 ), 0, CompletionItem::Entity ) ));
    items.insert( "chi", CompletionItem::Ptr(new CompletionItem( "chi", QChar( 967 ), 0, CompletionItem::Entity ) ));
    items.insert( "psi", CompletionItem::Ptr(new CompletionItem( "psi", QChar( 968 ), 0, CompletionItem::Entity ) ));
    items.insert( "omega", CompletionItem::Ptr(new CompletionItem( "omega", QChar( 969 ), 0, CompletionItem::Entity ) ));
    items.insert( "thetasym", CompletionItem::Ptr(new CompletionItem( "thetasym", QChar( 977 ), 0, CompletionItem::Entity ) ));
    items.insert( "upsih", CompletionItem::Ptr(new CompletionItem( "upsih", QChar( 978 ), 0, CompletionItem::Entity ) ));
    items.insert( "piv", CompletionItem::Ptr(new CompletionItem( "piv", QChar( 982 ), 0, CompletionItem::Entity ) ));
    items.insert( "ensp", CompletionItem::Ptr(new CompletionItem( "ensp", QChar( 8194 ), 0, CompletionItem::Entity ) ));
    items.insert( "emsp", CompletionItem::Ptr(new CompletionItem( "emsp", QChar( 8195 ), 0, CompletionItem::Entity ) ));
    items.insert( "thinsp", CompletionItem::Ptr(new CompletionItem( "thinsp", QChar( 8201 ), 0, CompletionItem::Entity ) ));
    items.insert( "zwnj", CompletionItem::Ptr(new CompletionItem( "zwnj", QChar( 8204 ), 0, CompletionItem::Entity ) ));
    items.insert( "zwj", CompletionItem::Ptr(new CompletionItem( "zwj", QChar( 8205 ), 0, CompletionItem::Entity ) ));
    items.insert( "lrm", CompletionItem::Ptr(new CompletionItem( "lrm", QChar( 8206 ), 0, CompletionItem::Entity ) ));
    items.insert( "rlm", CompletionItem::Ptr(new CompletionItem( "rlm", QChar( 8207 ), 0, CompletionItem::Entity ) ));
    items.insert( "ndash", CompletionItem::Ptr(new CompletionItem( "ndash", QChar( 8211 ), 0, CompletionItem::Entity ) ));
    items.insert( "mdash", CompletionItem::Ptr(new CompletionItem( "mdash", QChar( 8212 ), 0, CompletionItem::Entity ) ));
    items.insert( "lsquo", CompletionItem::Ptr(new CompletionItem( "lsquo", QChar( 8216 ), 0, CompletionItem::Entity ) ));
    items.insert( "rsquo", CompletionItem::Ptr(new CompletionItem( "rsquo", QChar( 8217 ), 0, CompletionItem::Entity ) ));
    items.insert( "sbquo", CompletionItem::Ptr(new CompletionItem( "sbquo", QChar( 8218 ), 0, CompletionItem::Entity ) ));
    items.insert( "ldquo", CompletionItem::Ptr(new CompletionItem( "ldquo", QChar( 8220 ), 0, CompletionItem::Entity ) ));
    items.insert( "rdquo", CompletionItem::Ptr(new CompletionItem( "rdquo", QChar( 8221 ), 0, CompletionItem::Entity ) ));
    items.insert( "bdquo", CompletionItem::Ptr(new CompletionItem( "bdquo", QChar( 8222 ), 0, CompletionItem::Entity ) ));
    items.insert( "dagger", CompletionItem::Ptr(new CompletionItem( "dagger", QChar( 8224 ), 0, CompletionItem::Entity ) ));
    items.insert( "Dagger", CompletionItem::Ptr(new CompletionItem( "Dagger", QChar( 8225 ), 0, CompletionItem::Entity ) ));
    items.insert( "bull", CompletionItem::Ptr(new CompletionItem( "bull", QChar( 8226 ), 0, CompletionItem::Entity ) ));
    items.insert( "hellip", CompletionItem::Ptr(new CompletionItem( "hellip", QChar( 8230 ), 0, CompletionItem::Entity ) ));
    items.insert( "permil", CompletionItem::Ptr(new CompletionItem( "permil", QChar( 8240 ), 0, CompletionItem::Entity ) ));
    items.insert( "prime", CompletionItem::Ptr(new CompletionItem( "prime", QChar( 8242 ), 0, CompletionItem::Entity ) ));
    items.insert( "Prime", CompletionItem::Ptr(new CompletionItem( "Prime", QChar( 8243 ), 0, CompletionItem::Entity ) ));
    items.insert( "lsaquo", CompletionItem::Ptr(new CompletionItem( "lsaquo", QChar( 8249 ), 0, CompletionItem::Entity ) ));
    items.insert( "rsaquo", CompletionItem::Ptr(new CompletionItem( "rsaquo", QChar( 8250 ), 0, CompletionItem::Entity ) ));
    items.insert( "oline", CompletionItem::Ptr(new CompletionItem( "oline", QChar( 8254 ), 0, CompletionItem::Entity ) ));
    items.insert( "frasl", CompletionItem::Ptr(new CompletionItem( "frasl", QChar( 8260 ), 0, CompletionItem::Entity ) ));
    items.insert( "euro", CompletionItem::Ptr(new CompletionItem( "euro", QChar( 8364 ), 0, CompletionItem::Entity ) ));
    items.insert( "image", CompletionItem::Ptr(new CompletionItem( "image", QChar( 8465 ), 0, CompletionItem::Entity ) ));
    items.insert( "weierp", CompletionItem::Ptr(new CompletionItem( "weierp", QChar( 8472 ), 0, CompletionItem::Entity ) ));
    items.insert( "real", CompletionItem::Ptr(new CompletionItem( "real", QChar( 8476 ), 0, CompletionItem::Entity ) ));
    items.insert( "trade", CompletionItem::Ptr(new CompletionItem( "trade", QChar( 8482 ), 0, CompletionItem::Entity ) ));
    items.insert( "alefsym", CompletionItem::Ptr(new CompletionItem( "alefsym", QChar( 8501 ), 0, CompletionItem::Entity ) ));
    items.insert( "larr", CompletionItem::Ptr(new CompletionItem( "larr", QChar( 8592 ), 0, CompletionItem::Entity ) ));
    items.insert( "uarr", CompletionItem::Ptr(new CompletionItem( "uarr", QChar( 8593 ), 0, CompletionItem::Entity ) ));
    items.insert( "rarr", CompletionItem::Ptr(new CompletionItem( "rarr", QChar( 8594 ), 0, CompletionItem::Entity ) ));
    items.insert( "darr", CompletionItem::Ptr(new CompletionItem( "darr", QChar( 8595 ), 0, CompletionItem::Entity ) ));
    items.insert( "harr", CompletionItem::Ptr(new CompletionItem( "harr", QChar( 8596 ), 0, CompletionItem::Entity ) ));
    items.insert( "crarr", CompletionItem::Ptr(new CompletionItem( "crarr", QChar( 8629 ), 0, CompletionItem::Entity ) ));
    items.insert( "lArr", CompletionItem::Ptr(new CompletionItem( "lArr", QChar( 8656 ), 0, CompletionItem::Entity ) ));
    items.insert( "uArr", CompletionItem::Ptr(new CompletionItem( "uArr", QChar( 8657 ), 0, CompletionItem::Entity ) ));
    items.insert( "rArr", CompletionItem::Ptr(new CompletionItem( "rArr", QChar( 8658 ), 0, CompletionItem::Entity ) ));
    items.insert( "dArr", CompletionItem::Ptr(new CompletionItem( "dArr", QChar( 8659 ), 0, CompletionItem::Entity ) ));
    items.insert( "hArr", CompletionItem::Ptr(new CompletionItem( "hArr", QChar( 8660 ), 0, CompletionItem::Entity ) ));
    items.insert( "forall", CompletionItem::Ptr(new CompletionItem( "forall", QChar( 8704 ), 0, CompletionItem::Entity ) ));
    items.insert( "part", CompletionItem::Ptr(new CompletionItem( "part", QChar( 8706 ), 0, CompletionItem::Entity ) ));
    items.insert( "exist", CompletionItem::Ptr(new CompletionItem( "exist", QChar( 8707 ), 0, CompletionItem::Entity ) ));
    items.insert( "empty", CompletionItem::Ptr(new CompletionItem( "empty", QChar( 8709 ), 0, CompletionItem::Entity ) ));
    items.insert( "nabla", CompletionItem::Ptr(new CompletionItem( "nabla", QChar( 8711 ), 0, CompletionItem::Entity ) ));
    items.insert( "isin", CompletionItem::Ptr(new CompletionItem( "isin", QChar( 8712 ), 0, CompletionItem::Entity ) ));
    items.insert( "notin", CompletionItem::Ptr(new CompletionItem( "notin", QChar( 8713 ), 0, CompletionItem::Entity ) ));
    items.insert( "ni", CompletionItem::Ptr(new CompletionItem( "ni", QChar( 8715 ), 0, CompletionItem::Entity ) ));
    items.insert( "prod", CompletionItem::Ptr(new CompletionItem( "prod", QChar( 8719 ), 0, CompletionItem::Entity ) ));
    items.insert( "sum", CompletionItem::Ptr(new CompletionItem( "sum", QChar( 8721 ), 0, CompletionItem::Entity ) ));
    items.insert( "minus", CompletionItem::Ptr(new CompletionItem( "minus", QChar( 8722 ), 0, CompletionItem::Entity ) ));
    items.insert( "lowast", CompletionItem::Ptr(new CompletionItem( "lowast", QChar( 8727 ), 0, CompletionItem::Entity ) ));
    items.insert( "radic", CompletionItem::Ptr(new CompletionItem( "radic", QChar( 8730 ), 0, CompletionItem::Entity ) ));
    items.insert( "prop", CompletionItem::Ptr(new CompletionItem( "prop", QChar( 8733 ), 0, CompletionItem::Entity ) ));
    items.insert( "infin", CompletionItem::Ptr(new CompletionItem( "infin", QChar( 8734 ), 0, CompletionItem::Entity ) ));
    items.insert( "ang", CompletionItem::Ptr(new CompletionItem( "ang", QChar( 8736 ), 0, CompletionItem::Entity ) ));
    items.insert( "and", CompletionItem::Ptr(new CompletionItem( "and", QChar( 8743 ), 0, CompletionItem::Entity ) ));
    items.insert( "or", CompletionItem::Ptr(new CompletionItem( "or", QChar( 8744 ), 0, CompletionItem::Entity ) ));
    items.insert( "cap", CompletionItem::Ptr(new CompletionItem( "cap", QChar( 8745 ), 0, CompletionItem::Entity ) ));
    items.insert( "cup", CompletionItem::Ptr(new CompletionItem( "cup", QChar( 8746 ), 0, CompletionItem::Entity ) ));
    items.insert( "int", CompletionItem::Ptr(new CompletionItem( "int", QChar( 8747 ), 0, CompletionItem::Entity ) ));
    items.insert( "there4", CompletionItem::Ptr(new CompletionItem( "there4", QChar( 8756 ), 0, CompletionItem::Entity ) ));
    items.insert( "sim", CompletionItem::Ptr(new CompletionItem( "sim", QChar( 8764 ), 0, CompletionItem::Entity ) ));
    items.insert( "cong", CompletionItem::Ptr(new CompletionItem( "cong", QChar( 8773 ), 0, CompletionItem::Entity ) ));
    items.insert( "asymp", CompletionItem::Ptr(new CompletionItem( "asymp", QChar( 8776 ), 0, CompletionItem::Entity ) ));
    items.insert( "ne", CompletionItem::Ptr(new CompletionItem( "ne", QChar( 8800 ), 0, CompletionItem::Entity ) ));
    items.insert( "equiv", CompletionItem::Ptr(new CompletionItem( "equiv", QChar( 8801 ), 0, CompletionItem::Entity ) ));
    items.insert( "le", CompletionItem::Ptr(new CompletionItem( "le", QChar( 8804 ), 0, CompletionItem::Entity ) ));
    items.insert( "ge", CompletionItem::Ptr(new CompletionItem( "ge", QChar( 8805 ), 0, CompletionItem::Entity ) ));
    items.insert( "sub", CompletionItem::Ptr(new CompletionItem( "sub", QChar( 8834 ), 0, CompletionItem::Entity ) ));
    items.insert( "sup", CompletionItem::Ptr(new CompletionItem( "sup", QChar( 8835 ), 0, CompletionItem::Entity ) ));
    items.insert( "nsub", CompletionItem::Ptr(new CompletionItem( "nsub", QChar( 8836 ), 0, CompletionItem::Entity ) ));
    items.insert( "sube", CompletionItem::Ptr(new CompletionItem( "sube", QChar( 8838 ), 0, CompletionItem::Entity ) ));
    items.insert( "supe", CompletionItem::Ptr(new CompletionItem( "supe", QChar( 8839 ), 0, CompletionItem::Entity ) ));
    items.insert( "oplus", CompletionItem::Ptr(new CompletionItem( "oplus", QChar( 8853 ), 0, CompletionItem::Entity ) ));
    items.insert( "otimes", CompletionItem::Ptr(new CompletionItem( "otimes", QChar( 8855 ), 0, CompletionItem::Entity ) ));
    items.insert( "perp", CompletionItem::Ptr(new CompletionItem( "perp", QChar( 8869 ), 0, CompletionItem::Entity ) ));
    items.insert( "sdot", CompletionItem::Ptr(new CompletionItem( "sdot", QChar( 8901 ), 0, CompletionItem::Entity ) ));
    items.insert( "lceil", CompletionItem::Ptr(new CompletionItem( "lceil", QChar( 8968 ), 0, CompletionItem::Entity ) ));
    items.insert( "rceil", CompletionItem::Ptr(new CompletionItem( "rceil", QChar( 8969 ), 0, CompletionItem::Entity ) ));
    items.insert( "lfloor", CompletionItem::Ptr(new CompletionItem( "lfloor", QChar( 8970 ), 0, CompletionItem::Entity ) ));
    items.insert( "rfloor", CompletionItem::Ptr(new CompletionItem( "rfloor", QChar( 8971 ), 0, CompletionItem::Entity ) ));
    items.insert( "lang", CompletionItem::Ptr(new CompletionItem( "lang", QChar( 9001 ), 0, CompletionItem::Entity ) ));
    items.insert( "rang", CompletionItem::Ptr(new CompletionItem( "rang", QChar( 9002 ), 0, CompletionItem::Entity ) ));
    items.insert( "loz", CompletionItem::Ptr(new CompletionItem( "loz", QChar( 9674 ), 0, CompletionItem::Entity ) ));
    items.insert( "spades", CompletionItem::Ptr(new CompletionItem( "spades", QChar( 9824 ), 0, CompletionItem::Entity ) ));
    items.insert( "clubs", CompletionItem::Ptr(new CompletionItem( "clubs", QChar( 9827 ), 0, CompletionItem::Entity ) ));
    items.insert( "hearts", CompletionItem::Ptr(new CompletionItem( "hearts", QChar( 9829 ), 0, CompletionItem::Entity ) ));
    items.insert( "diams", CompletionItem::Ptr(new CompletionItem( "diams", QChar( 9830 ), 0, CompletionItem::Entity ) ));
    return items.values();
}


