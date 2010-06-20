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

#include "sgmlcodecompletionmodel.h"
#include "elementdeclaration.h"
#include "../language_debug.h"

#include "../duchain/sgmldefaultvisitor.h"
#include "../duchain/parser/editorintegrator.h"
#include "../duchain/parser/parsesession.h"

#include <QtGui/QIcon>

#include <KDE/KTextEditor/View>
#include <KDE/KTextEditor/Document>
#include <KDE/KMimeType>

#include <language/duchain/topducontext.h>
#include <language/duchain/duchainlock.h>
#include <language/duchain/duchain.h>
#include <language/duchain/declaration.h>
#include <language/duchain/classdeclaration.h>

#include <interfaces/icore.h>
#include <interfaces/isourceformattercontroller.h>
#include <interfaces/isourceformatter.h>


using namespace Xml;

using namespace KDevelop;
using namespace KTextEditor;

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
        if (editor->parseSession()->tokenStream()->token(node->endToken).kind == Parser::Token_GT
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
        element = 0;
        DefaultVisitor::visitElementCloseTag(node);
    }

    QStack<ElementTagAst *> contextStack;


    QString contextName() {
        if(!context) return QString();
        return nodeText(context->name);
    }

    QString contextPrettyName() {
        if(context->ns)
            return QString("%1:%2").arg(nodeText(context->ns), nodeText(context->name));
        return nodeText(context->name);
    }

    QString elementName() {
        if(!element) return QString();
        return nodeText(element->name);
    }

    QString elementPrettyName() {
        if(element->ns)
            return QString("%1:%2").arg(nodeText(element->ns), nodeText(element->name));
        return nodeText(element->name);
    }

    QString attributeName() {
        if(!attribute) return QString();
        return nodeText(attribute->name);
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
};

SgmlCodeCompletionModel::SgmlCodeCompletionModel(QObject* parent): CodeCompletionModel2(parent) {
    m_depth = 0;
}

SgmlCodeCompletionModel::~SgmlCodeCompletionModel() {}

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


void SgmlCodeCompletionModel::completionInvoked(KTextEditor::View* view, const KTextEditor::Range& range, KTextEditor::CodeCompletionModel::InvocationType invocationType)
{
    KUrl url = view->document()->url();
    QString text;
    setRowCount(0);
    m_items.clear();
    m_depth = 0;

    //TODO Hack start
    //If the text is long this could take for ever
    //If the element declaration runs more than 20 lines
    //this does not work.
    Range r = range;
    r.start().setColumn(0);
    if (r.start().line() < 20)
        r.start().setLine(0);
    else
        r.start().setLine(r.start().line() - 20);
    //Prevent very long text
    if (r.end().column() > 1000)
        r.start().setColumn(500);
    text = view->document()->text(r).remove('\r').remove('\n');

    QRegExp expTag(".*([</>!]+)\\s*[\\w:]*$");
    expTag.exactMatch(text);
    QRegExp expAtt(".*[</]+\\s*(\\w+\\s*:\\s*)?(\\w+)\\s+[^<>]*$");
    expAtt.exactMatch(text);
    QRegExp expValue(".*(\\w+\\s*)=(\\s*([\"']\\s*\\w*[\"']?)?(\\w+)?)$");
    expValue.exactMatch(text);
    QRegExp expEntity(".*([&]\\w*)$");
    expEntity.exactMatch(text);

    QString esc = expTag.cap(1);
    QString tag = expAtt.cap(2);
    QString att = expValue.cap(2);
    QString entity = expEntity.cap(1);
    //TODO Hack end

    //Tokenize
    ParseSession session;
    Range rng = range;
    rng.start().setLine(0);
    rng.start().setColumn(0);
    session.setContents(view->document()->text(rng));
    session.setCurrentDocument(view->document()->url().pathOrUrl());
    session.setMime(KMimeType::mimeType(view->document()->mimeType()));
    StartAst *start=0;
    if (!session.parse(&start)) {
        debug() << "Failed to parse content";
    }
    Xml::EditorIntegrator editor(&session);
    CompletionVisitor visitor(&editor);
    visitor.visitNode(start);
    m_depth = visitor.contextStack.size();
    
    if (visitor.attribute) {
       debug() << "# attribute" << visitor.nodeText(visitor.attribute->name);
    } else {
        debug() << "# attribute null";
    }
    if (visitor.context) {
        debug() << "# context" << visitor.nodeText(visitor.context->name);
    } else {
        debug() << "# context null";
    }
    if (visitor.element) {
        debug() << "# element" << visitor.nodeText(visitor.element->name);
    } else {
        debug() << "# element null";
    }

    if (text.trimmed().isEmpty()) {
        m_items.append(findHeadersForDocument(view->document()));
    }

    if (esc.endsWith('!') && tag.isEmpty()) {
        m_items.append(CompletionItem("DOCTYPE", 10, Other));
        m_items.append(CompletionItem("ELEMENT", 10, Other));
        m_items.append(CompletionItem("ENTITY", 10, Other));
        m_items.append(CompletionItem("ATTLIST", 10, Other));
        m_items.append(CompletionItem("NOTATION", 10, Other));
        m_items.append(CompletionItem("[CDATA[", 0, Other));
        m_items.append(CompletionItem("[PCDATA[", 0, Other));
        setRowCount(m_items.size());
        return;
    }

    if (!entity.isEmpty()) {
        m_items.append(findAllEntities(view->document(), range));
        setRowCount(m_items.size());
        return;
    }

    //Complete enumerations
    if(visitor.attribute && !att.isEmpty()) {
        //TODO
    }
    //Complete attributes
    else if(visitor.element && !tag.isEmpty()) {
        m_items.append(findAttributes(view->document(), range, visitor.elementName()));
    }
    //Complete tags
    else if(visitor.context) {
        if((esc.endsWith("/") || esc.endsWith(">")) && !esc.endsWith("/>") || esc.startsWith("</"))
            m_items.append(CompletionItem(QString("/%1").arg(visitor.contextPrettyName()), 10, Element));
        QHash<QString, CompletionItem> items;
        QList<CompletionItem> list = findChildElements(view->document(), range, visitor.contextName());
        foreach(const CompletionItem &i, list) {
                items.insert(i.name, i);
        }
        list = findAll(view->document(), range);
        foreach(const CompletionItem &i, list) {
            if(!items.contains(i.name)) {
                items.insert(i.name, i);
            }
        }
        m_items.append(items.values());
    }
    //Add all
    else {
        m_items.append(findAll(view->document(), range));
    }
    
    setRowCount(m_items.count());
}

KTextEditor::Range SgmlCodeCompletionModel::completionRange(KTextEditor::View* view, const KTextEditor::Cursor& position)
{
    const static QString seps = "/\\!<>:= \r\n\t[]()'\";&";
    KTextEditor::Cursor start = position;
    KTextEditor::Cursor end = position;
    QString text = view->document()->line(position.line());
    if ( text.isEmpty() || start.column() >= text.length() ) {
      return Range(start, end);
    }
    while (start.column() > 0 && !seps.contains(text.at(start.column()))) {
        start.setColumn(start.column()-1);
        if (seps.contains(text.at(start.column()))) {
            start.setColumn(start.column()+1);
            break;
        }
    }
    while (end.column() < text.length() && !seps.contains(text.at(end.column()))) {
        end.setColumn(end.column()+1);
        if (seps.contains(text.at(end.column()))) {
            end.setColumn(end.column());
            break;
        }
    }
    return Range(start, end);
}

const QIcon SgmlCodeCompletionModel::getIcon ( SgmlCodeCompletionModel::CompletionItemType type ) const
{
    static QHash <SgmlCodeCompletionModel::CompletionItemType, QIcon> icons;
    if ( icons.contains ( type ) )
        return icons[type];
    QString name;
    if (type == Element)
        name = "element";
    else if (type == Attribute)
        name = "attribute";
    else if (type == Entity)
        name = "entity";
    else if (type == Enum)
        name = "enum";
    if (name.isEmpty())
        return QIcon();
    QIcon i ( QString ( ":/xml/completion/%1.png" ).arg ( name ) );
    icons.insert ( type, i );
    return icons[type];
}

QVariant SgmlCodeCompletionModel::data(const QModelIndex& index, int role) const
{
    if (index.parent().isValid())
        return QVariant();
    if (m_items.count() <= index.row())
        return QVariant();

    if (role == Qt::DisplayRole && index.column() == CodeCompletionModel::Name)
        return QVariant(m_items.at(index.row()).name);

    if (role == Qt::DisplayRole && index.column() == CodeCompletionModel::Postfix)
        return QVariant(m_items.at(index.row()).info);

    if (role == MatchQuality)
        return QVariant(m_items.at(index.row()).matchLevel);

    if (role == InheritanceDepth)
        return QVariant(10 - m_items.at(index.row()).matchLevel + 1);

    if (role == Qt::DecorationRole && index.column() == CodeCompletionModel::Icon)
        return getIcon(m_items.at(index.row()).type);

    return QVariant();
}


QString SgmlCodeCompletionModel::getIndentstring(Document* document, int depth) const
{
    int indentLength = 4;
    ISourceFormatter::IndentationType indentType = ISourceFormatter::IndentWithSpaces;
    ISourceFormatter * formatter = 0;
    KMimeType::Ptr mime = KMimeType::mimeType(document->mimeType());
    formatter = ICore::self()->sourceFormatterController()->formatterForMimeType(mime);
    if (formatter) {
        indentLength = formatter->indentationLength();
        indentType = formatter->indentationType();
    }
    if (indentType == ISourceFormatter::NoChange)
        return "";

    QChar c = ' ';
    if (indentType == ISourceFormatter::IndentWithTabs) {
        c = '\t';
    }

    QString ret;
    for (int i = 0; i < depth * indentLength; i++)
        ret += c;
    return ret;
}

QString SgmlCodeCompletionModel::formatString(Document* document, const QString& str, CompletionItemType type) const
{
    if (type != Element)
        return str;
    
    //TODO Need to fix SourceFormatterController, on my system it does not save/use my preferences.
    /*
    KMimeType::Ptr mime = KMimeType::mimeType(document->mimeType());
    ISourceFormatter *formatter = ICore::self()->sourceFormatterController()->formatterForMimeType(mime);
    SourceFormatterStyle style = ICore::self()->sourceFormatterController()->styleForMimeType(mime);
    if(formatter) {
        kDebug() << style.name();
        kDebug() << style.caption();
        QMap<QString, QVariant> map = formatter->stringToOptionMap(style.content());
        if(map.contains("CASE")) {
            if(map.value("CASE") == "LOWER")
                return str.toLower();
            if(map.value("CASE") == "UPPER")
                return str.toUpper();
        }
    }
    return str;
    */
    
    
    #define CASESENSITIVE 0
    #define LOWERCASE 1
    #define UPPERCASE 1
    
    if (type != Element)
    return str;
    
    KMimeType::Ptr mime = KMimeType::mimeType(document->mimeType());
    
    if (mime->is ( "application/xml" ) ||
        mime->is ( "application/xslt+xml" ) ||
        mime->is ( "application/xsd" ) ||
        mime->is ( "application/wsdl+xml" ) ||
        mime->is ( "application/x-wsdl" )) {
        return str;
    }

    int lowerCaseCount = 0;
    int upperCaseCount = 0;
    {
        DUChainReadLocker lock;
        TopDUContext * tc = 0;
        QHash < QString,  CompletionItem > items;
        tc = DUChain::self()->chainForDocument(document->url());
        if (!tc)
            return str;
        QList<Declaration *> decs;
        foreach(Declaration * dec, tc->localDeclarations()) {
            if (dec->kind() != Declaration::Instance)
                continue;
            decs.append(dec);
            if (dec->internalContext()) {
                foreach(Declaration * d, dec->internalContext()->localDeclarations()) {
                    decs.append(d);
                }
            }
        }
        foreach(Declaration * dec, decs) {
            if (dec->kind() == Declaration::Instance) {
                ElementDeclaration *elementDec = dynamic_cast<ElementDeclaration *>(dec);
                if (elementDec) {
                    QString name = elementDec->name().str();
                    int lowerCount = 0;
                    int upperCount = 0;
                    for (int i =0; i < name.size(); i++) {
                        if (name[i].isLower())
                            lowerCount++;
                        if (name[i].isUpper())
                            upperCount++;
                    }
                    if (lowerCount == name.length())
                        lowerCaseCount++;
                    if (upperCount == name.length())
                        upperCaseCount++;
                }
            }
        }
    }
    
    if (lowerCaseCount && !upperCaseCount) {
        return str.toLower();
    }

    if (upperCaseCount && !lowerCaseCount) {
        return str.toUpper();
    }
    
    return str;
}

Range SgmlCodeCompletionModel::growRangeLeft(Document* document, const Range& range, const QString& condition) const
{
    Range r = range;
    while (r.start().column() > 0) {
        r.start().setColumn(r.start().column() -1);
        QChar c = document->character(r.start());
        if (condition.contains(c))
            continue;
        r.start().setColumn(r.start().column() + 1);
        break;
    }
    return r;
}

Range SgmlCodeCompletionModel::growRangeRight(Document* document, const Range& range, const QString& condition) const
{
    Range r = range;
    QString line = document->line(range.end().line());
    while (r.end().column() < line.size()) {
        QChar c = document->character(r.end());
        if (condition.contains(c)) {
            r.end().setColumn(r.end().column() +1);
            continue;
        }
        break;
    }
    return r;
}

QChar SgmlCodeCompletionModel::getSeperator(Document* document, const KTextEditor::Cursor& position) const
{
    static QString seps = "</:!>=\"\'&";
    QString line = document->line(position.line());
    KTextEditor::Cursor pos = position;
    QChar posc;
    int posi = position.column();
    while (posi-- > 0) {
        pos.setColumn(posi);
        posc = document->character(pos);
        if (seps.contains(posc)) {
            return posc;
        }
        if (posc.isSpace())
            continue;
        return QChar('\0');
    }
    return QChar('\0');
}


void SgmlCodeCompletionModel::executeCompletionItem2(KTextEditor::Document* document, const KTextEditor::Range& word, const QModelIndex& index) const
{
    CompletionItem item = m_items.at(index.row());
    QString text = formatString(document, item.name, item.type);
    QString line = document->line(word.start().line());
    QString trimmedLine = line.mid(0, word.start().column()).remove('/');
    trimmedLine = trimmedLine.remove('<');
    trimmedLine = trimmedLine.remove('>').trimmed();
    int depth = m_depth;
    QChar seperator = getSeperator(document, word.start());


    //Close tag
    //After replacement move cursur to start of context ie <blah>|</blah>
    if (seperator == '/' || text.startsWith('/')) {
        if (text.startsWith('/'))
            text = text.mid(1, text.size() -1);
        Range range = word;
        range = growRangeLeft(document, range, "</");
        debug() << "right1" << (range.end().column() < line.length() ? line.at(range.end().column()) : ' ');
        range = growRangeRight(document, range, ">");
        debug() << "right2" << (range.end().column() < line.length() ? line.at(range.end().column()) : ' ');
        if (trimmedLine.isEmpty()) {
            Range r = range;
            r.start().setColumn(0);
            text = QString("%1</%2>").arg(getIndentstring(document, depth-1),text);
            document->replaceText(r, text);
        } else {
            text = QString("</%1>").arg(text);
            document->replaceText(range, text);
        }
        foreach(View * v, document->views()) {
            if (v->isActiveView())
                v->setCursorPosition(range.start());
        }
        return;
    }

    Range range = word;

    //Attributes
    if (item.type == Attribute) {
        // overwrite existing attribute to the left
        Cursor pos = range.start();
        while ( pos.column() > 1 && !document->character(pos - Cursor(0, 1)).isSpace() ) {
          pos.setColumn(pos.column() - 1);
        }
        range.start() = pos;
        range = growRangeRight(document, range, "=\"'");
        QChar endChar;
        if (range.end().column() > 0) {
          endChar = document->character(range.end() - Cursor(0, 1));
        }
        bool addQuote = endChar != '\'' && endChar != '"';
        if (addQuote && range.end().column() > 1) {
          endChar = document->character(range.end() - Cursor(0, 2));
        }
        bool addEquals = endChar != '=';

        if (addEquals) {
          text += '=';
        }
        if (addQuote) {
          text += "\"\"";
        } else {
          // keep existing quote
          text += document->character(range.end() - Cursor(0, 1));
        }
        document->replaceText(range, text);

        // place cursor between quotes
        range.end().setColumn(range.start().column() + text.size() - (addQuote ? 1 : 0));
        foreach(View * v, document->views()) {
            if (v->isActiveView())
                v->setCursorPosition(range.end());
        }
        return;
    }

    //Entities
    if (item.type == Entity) {
        range = growRangeRight(document, range, ";");
        range = growRangeLeft(document, range, "&");
        text = QString("&%1;").arg(text);
        document->replaceText(range, text);
        return;
    }

    //Elements
    if ((seperator == '<' || seperator == '>') || seperator.isNull()) {
        range = growRangeLeft(document, range, "<");
        if (!text.startsWith('<')) {
          text.prepend('<');
        }
        if (trimmedLine.isEmpty()) {
            range.start().setColumn(0);
            text.prepend(getIndentstring(document, depth));
        }
        document->replaceText(range, text);
    } else {
        text = QString("%1").arg(text);
        document->replaceText(range, text);
    }

    //After replacement move cursur to end of tag ie: <blah>|
    range.end().setColumn(range.start().column() + text.size());
    QString growSeps = "=\"'/>";
    range = growRangeRight(document, range, growSeps);
    if (growSeps.contains(document->character(range.end())))
        range.end().setColumn(range.end().column() + 1);
    foreach(View * v, document->views()) {
        if (v->isActiveView())
            v->setCursorPosition(range.end());
    }
}

bool SgmlCodeCompletionModel::shouldAbortCompletion(KTextEditor::View* view, const KTextEditor::SmartRange& range, const QString& currentCompletion)
{
    return false;
}

bool SgmlCodeCompletionModel::shouldStartCompletion(View* view, const QString& insertedText, bool userInsertion, const KTextEditor::Cursor& position)
{
    return !getSeperator(view->document(), position).isNull();
}


QList< SgmlCodeCompletionModel::CompletionItem > SgmlCodeCompletionModel::findAllEntities(Document* document, const KTextEditor::Range& range) const
{
    DUChainReadLocker lock;
    TopDUContext * tc = 0;
    QHash < QString,  CompletionItem > items;

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
            items.insert(name, CompletionItem(name, info, 0, Entity));
        }
    }

    //Some standard entities
    items.insert( "quot", CompletionItem( "quot", QChar( 34 ), 0, Entity ) );
    items.insert( "amp", CompletionItem( "amp", QChar( 38 ), 0, Entity ) );
    items.insert( "apos", CompletionItem( "apos", QChar( 39 ), 0, Entity ) );
    items.insert( "lt", CompletionItem( "lt", QChar( 60 ), 0, Entity ) );
    items.insert( "gt", CompletionItem( "gt", QChar( 62 ), 0, Entity ) );
    items.insert( "nbsp", CompletionItem( "nbsp", QChar( 160 ), 0, Entity ) );
    items.insert( "iexcl", CompletionItem( "iexcl", QChar( 161 ), 0, Entity ) );
    items.insert( "cent", CompletionItem( "cent", QChar( 162 ), 0, Entity ) );
    items.insert( "pound", CompletionItem( "pound", QChar( 163 ), 0, Entity ) );
    items.insert( "curren", CompletionItem( "curren", QChar( 164 ), 0, Entity ) );
    items.insert( "yen", CompletionItem( "yen", QChar( 165 ), 0, Entity ) );
    items.insert( "brvbar", CompletionItem( "brvbar", QChar( 166 ), 0, Entity ) );
    items.insert( "sect", CompletionItem( "sect", QChar( 167 ), 0, Entity ) );
    items.insert( "uml", CompletionItem( "uml", QChar( 168 ), 0, Entity ) );
    items.insert( "copy", CompletionItem( "copy", QChar( 169 ), 0, Entity ) );
    items.insert( "ordf", CompletionItem( "ordf", QChar( 170 ), 0, Entity ) );
    items.insert( "laquo", CompletionItem( "laquo", QChar( 171 ), 0, Entity ) );
    items.insert( "not", CompletionItem( "not", QChar( 172 ), 0, Entity ) );
    items.insert( "shy", CompletionItem( "shy", QChar( 173 ), 0, Entity ) );
    items.insert( "reg", CompletionItem( "reg", QChar( 174 ), 0, Entity ) );
    items.insert( "macr", CompletionItem( "macr", QChar( 175 ), 0, Entity ) );
    items.insert( "deg", CompletionItem( "deg", QChar( 176 ), 0, Entity ) );
    items.insert( "plusmn", CompletionItem( "plusmn", QChar( 177 ), 0, Entity ) );
    items.insert( "sup2", CompletionItem( "sup2", QChar( 178 ), 0, Entity ) );
    items.insert( "sup3", CompletionItem( "sup3", QChar( 179 ), 0, Entity ) );
    items.insert( "acute", CompletionItem( "acute", QChar( 180 ), 0, Entity ) );
    items.insert( "micro", CompletionItem( "micro", QChar( 181 ), 0, Entity ) );
    items.insert( "para", CompletionItem( "para", QChar( 182 ), 0, Entity ) );
    items.insert( "middot", CompletionItem( "middot", QChar( 183 ), 0, Entity ) );
    items.insert( "cedil", CompletionItem( "cedil", QChar( 184 ), 0, Entity ) );
    items.insert( "sup1", CompletionItem( "sup1", QChar( 185 ), 0, Entity ) );
    items.insert( "ordm", CompletionItem( "ordm", QChar( 186 ), 0, Entity ) );
    items.insert( "raquo", CompletionItem( "raquo", QChar( 187 ), 0, Entity ) );
    items.insert( "frac14", CompletionItem( "frac14", QChar( 188 ), 0, Entity ) );
    items.insert( "frac12", CompletionItem( "frac12", QChar( 189 ), 0, Entity ) );
    items.insert( "frac34", CompletionItem( "frac34", QChar( 190 ), 0, Entity ) );
    items.insert( "iquest", CompletionItem( "iquest", QChar( 191 ), 0, Entity ) );
    items.insert( "Agrave", CompletionItem( "Agrave", QChar( 192 ), 0, Entity ) );
    items.insert( "Aacute", CompletionItem( "Aacute", QChar( 193 ), 0, Entity ) );
    items.insert( "Acirc", CompletionItem( "Acirc", QChar( 194 ), 0, Entity ) );
    items.insert( "Atilde", CompletionItem( "Atilde", QChar( 195 ), 0, Entity ) );
    items.insert( "Auml", CompletionItem( "Auml", QChar( 196 ), 0, Entity ) );
    items.insert( "Aring", CompletionItem( "Aring", QChar( 197 ), 0, Entity ) );
    items.insert( "AElig", CompletionItem( "AElig", QChar( 198 ), 0, Entity ) );
    items.insert( "Ccedil", CompletionItem( "Ccedil", QChar( 199 ), 0, Entity ) );
    items.insert( "Egrave", CompletionItem( "Egrave", QChar( 200 ), 0, Entity ) );
    items.insert( "Eacute", CompletionItem( "Eacute", QChar( 201 ), 0, Entity ) );
    items.insert( "Ecirc", CompletionItem( "Ecirc", QChar( 202 ), 0, Entity ) );
    items.insert( "Euml", CompletionItem( "Euml", QChar( 203 ), 0, Entity ) );
    items.insert( "Igrave", CompletionItem( "Igrave", QChar( 204 ), 0, Entity ) );
    items.insert( "Iacute", CompletionItem( "Iacute", QChar( 205 ), 0, Entity ) );
    items.insert( "Icirc", CompletionItem( "Icirc", QChar( 206 ), 0, Entity ) );
    items.insert( "Iuml", CompletionItem( "Iuml", QChar( 207 ), 0, Entity ) );
    items.insert( "ETH", CompletionItem( "ETH", QChar( 208 ), 0, Entity ) );
    items.insert( "Ntilde", CompletionItem( "Ntilde", QChar( 209 ), 0, Entity ) );
    items.insert( "Ograve", CompletionItem( "Ograve", QChar( 210 ), 0, Entity ) );
    items.insert( "Oacute", CompletionItem( "Oacute", QChar( 211 ), 0, Entity ) );
    items.insert( "Ocirc", CompletionItem( "Ocirc", QChar( 212 ), 0, Entity ) );
    items.insert( "Otilde", CompletionItem( "Otilde", QChar( 213 ), 0, Entity ) );
    items.insert( "Ouml", CompletionItem( "Ouml", QChar( 214 ), 0, Entity ) );
    items.insert( "times", CompletionItem( "times", QChar( 215 ), 0, Entity ) );
    items.insert( "Oslash", CompletionItem( "Oslash", QChar( 216 ), 0, Entity ) );
    items.insert( "Ugrave", CompletionItem( "Ugrave", QChar( 217 ), 0, Entity ) );
    items.insert( "Uacute", CompletionItem( "Uacute", QChar( 218 ), 0, Entity ) );
    items.insert( "Ucirc", CompletionItem( "Ucirc", QChar( 219 ), 0, Entity ) );
    items.insert( "Uuml", CompletionItem( "Uuml", QChar( 220 ), 0, Entity ) );
    items.insert( "Yacute", CompletionItem( "Yacute", QChar( 221 ), 0, Entity ) );
    items.insert( "THORN", CompletionItem( "THORN", QChar( 222 ), 0, Entity ) );
    items.insert( "szlig", CompletionItem( "szlig", QChar( 223 ), 0, Entity ) );
    items.insert( "agrave", CompletionItem( "agrave", QChar( 224 ), 0, Entity ) );
    items.insert( "aacute", CompletionItem( "aacute", QChar( 225 ), 0, Entity ) );
    items.insert( "acirc", CompletionItem( "acirc", QChar( 226 ), 0, Entity ) );
    items.insert( "atilde", CompletionItem( "atilde", QChar( 227 ), 0, Entity ) );
    items.insert( "auml", CompletionItem( "auml", QChar( 228 ), 0, Entity ) );
    items.insert( "aring", CompletionItem( "aring", QChar( 229 ), 0, Entity ) );
    items.insert( "aelig", CompletionItem( "aelig", QChar( 230 ), 0, Entity ) );
    items.insert( "ccedil", CompletionItem( "ccedil", QChar( 231 ), 0, Entity ) );
    items.insert( "egrave", CompletionItem( "egrave", QChar( 232 ), 0, Entity ) );
    items.insert( "eacute", CompletionItem( "eacute", QChar( 233 ), 0, Entity ) );
    items.insert( "ecirc", CompletionItem( "ecirc", QChar( 234 ), 0, Entity ) );
    items.insert( "euml", CompletionItem( "euml", QChar( 235 ), 0, Entity ) );
    items.insert( "igrave", CompletionItem( "igrave", QChar( 236 ), 0, Entity ) );
    items.insert( "iacute", CompletionItem( "iacute", QChar( 237 ), 0, Entity ) );
    items.insert( "icirc", CompletionItem( "icirc", QChar( 238 ), 0, Entity ) );
    items.insert( "iuml", CompletionItem( "iuml", QChar( 239 ), 0, Entity ) );
    items.insert( "eth", CompletionItem( "eth", QChar( 240 ), 0, Entity ) );
    items.insert( "ntilde", CompletionItem( "ntilde", QChar( 241 ), 0, Entity ) );
    items.insert( "ograve", CompletionItem( "ograve", QChar( 242 ), 0, Entity ) );
    items.insert( "oacute", CompletionItem( "oacute", QChar( 243 ), 0, Entity ) );
    items.insert( "ocirc", CompletionItem( "ocirc", QChar( 244 ), 0, Entity ) );
    items.insert( "otilde", CompletionItem( "otilde", QChar( 245 ), 0, Entity ) );
    items.insert( "ouml", CompletionItem( "ouml", QChar( 246 ), 0, Entity ) );
    items.insert( "divide", CompletionItem( "divide", QChar( 247 ), 0, Entity ) );
    items.insert( "oslash", CompletionItem( "oslash", QChar( 248 ), 0, Entity ) );
    items.insert( "ugrave", CompletionItem( "ugrave", QChar( 249 ), 0, Entity ) );
    items.insert( "uacute", CompletionItem( "uacute", QChar( 250 ), 0, Entity ) );
    items.insert( "ucirc", CompletionItem( "ucirc", QChar( 251 ), 0, Entity ) );
    items.insert( "uuml", CompletionItem( "uuml", QChar( 252 ), 0, Entity ) );
    items.insert( "yacute", CompletionItem( "yacute", QChar( 253 ), 0, Entity ) );
    items.insert( "thorn", CompletionItem( "thorn", QChar( 254 ), 0, Entity ) );
    items.insert( "yuml", CompletionItem( "yuml", QChar( 255 ), 0, Entity ) );
    items.insert( "OElig", CompletionItem( "OElig", QChar( 338 ), 0, Entity ) );
    items.insert( "oelig", CompletionItem( "oelig", QChar( 339 ), 0, Entity ) );
    items.insert( "Scaron", CompletionItem( "Scaron", QChar( 352 ), 0, Entity ) );
    items.insert( "scaron", CompletionItem( "scaron", QChar( 353 ), 0, Entity ) );
    items.insert( "Yuml", CompletionItem( "Yuml", QChar( 376 ), 0, Entity ) );
    items.insert( "fnof", CompletionItem( "fnof", QChar( 402 ), 0, Entity ) );
    items.insert( "circ", CompletionItem( "circ", QChar( 710 ), 0, Entity ) );
    items.insert( "tilde", CompletionItem( "tilde", QChar( 732 ), 0, Entity ) );
    items.insert( "Alpha", CompletionItem( "Alpha", QChar( 913 ), 0, Entity ) );
    items.insert( "Beta", CompletionItem( "Beta", QChar( 914 ), 0, Entity ) );
    items.insert( "Gamma", CompletionItem( "Gamma", QChar( 915 ), 0, Entity ) );
    items.insert( "Delta", CompletionItem( "Delta", QChar( 916 ), 0, Entity ) );
    items.insert( "Epsilon", CompletionItem( "Epsilon", QChar( 917 ), 0, Entity ) );
    items.insert( "Zeta", CompletionItem( "Zeta", QChar( 918 ), 0, Entity ) );
    items.insert( "Eta", CompletionItem( "Eta", QChar( 919 ), 0, Entity ) );
    items.insert( "Theta", CompletionItem( "Theta", QChar( 920 ), 0, Entity ) );
    items.insert( "Iota", CompletionItem( "Iota", QChar( 921 ), 0, Entity ) );
    items.insert( "Kappa", CompletionItem( "Kappa", QChar( 922 ), 0, Entity ) );
    items.insert( "Lambda", CompletionItem( "Lambda", QChar( 923 ), 0, Entity ) );
    items.insert( "Mu", CompletionItem( "Mu", QChar( 924 ), 0, Entity ) );
    items.insert( "Nu", CompletionItem( "Nu", QChar( 925 ), 0, Entity ) );
    items.insert( "Xi", CompletionItem( "Xi", QChar( 926 ), 0, Entity ) );
    items.insert( "Omicron", CompletionItem( "Omicron", QChar( 927 ), 0, Entity ) );
    items.insert( "Pi", CompletionItem( "Pi", QChar( 928 ), 0, Entity ) );
    items.insert( "Rho", CompletionItem( "Rho", QChar( 929 ), 0, Entity ) );
    items.insert( "Sigma", CompletionItem( "Sigma", QChar( 931 ), 0, Entity ) );
    items.insert( "Tau", CompletionItem( "Tau", QChar( 932 ), 0, Entity ) );
    items.insert( "Upsilon", CompletionItem( "Upsilon", QChar( 933 ), 0, Entity ) );
    items.insert( "Phi", CompletionItem( "Phi", QChar( 934 ), 0, Entity ) );
    items.insert( "Chi", CompletionItem( "Chi", QChar( 935 ), 0, Entity ) );
    items.insert( "Psi", CompletionItem( "Psi", QChar( 936 ), 0, Entity ) );
    items.insert( "Omega", CompletionItem( "Omega", QChar( 937 ), 0, Entity ) );
    items.insert( "alpha", CompletionItem( "alpha", QChar( 945 ), 0, Entity ) );
    items.insert( "beta", CompletionItem( "beta", QChar( 946 ), 0, Entity ) );
    items.insert( "gamma", CompletionItem( "gamma", QChar( 947 ), 0, Entity ) );
    items.insert( "delta", CompletionItem( "delta", QChar( 948 ), 0, Entity ) );
    items.insert( "epsilon", CompletionItem( "epsilon", QChar( 949 ), 0, Entity ) );
    items.insert( "zeta", CompletionItem( "zeta", QChar( 950 ), 0, Entity ) );
    items.insert( "eta", CompletionItem( "eta", QChar( 951 ), 0, Entity ) );
    items.insert( "theta", CompletionItem( "theta", QChar( 952 ), 0, Entity ) );
    items.insert( "iota", CompletionItem( "iota", QChar( 953 ), 0, Entity ) );
    items.insert( "kappa", CompletionItem( "kappa", QChar( 954 ), 0, Entity ) );
    items.insert( "lambda", CompletionItem( "lambda", QChar( 955 ), 0, Entity ) );
    items.insert( "mu", CompletionItem( "mu", QChar( 956 ), 0, Entity ) );
    items.insert( "nu", CompletionItem( "nu", QChar( 957 ), 0, Entity ) );
    items.insert( "xi", CompletionItem( "xi", QChar( 958 ), 0, Entity ) );
    items.insert( "omicron", CompletionItem( "omicron", QChar( 959 ), 0, Entity ) );
    items.insert( "pi", CompletionItem( "pi", QChar( 960 ), 0, Entity ) );
    items.insert( "rho", CompletionItem( "rho", QChar( 961 ), 0, Entity ) );
    items.insert( "sigmaf", CompletionItem( "sigmaf", QChar( 962 ), 0, Entity ) );
    items.insert( "sigma", CompletionItem( "sigma", QChar( 963 ), 0, Entity ) );
    items.insert( "tau", CompletionItem( "tau", QChar( 964 ), 0, Entity ) );
    items.insert( "upsilon", CompletionItem( "upsilon", QChar( 965 ), 0, Entity ) );
    items.insert( "phi", CompletionItem( "phi", QChar( 966 ), 0, Entity ) );
    items.insert( "chi", CompletionItem( "chi", QChar( 967 ), 0, Entity ) );
    items.insert( "psi", CompletionItem( "psi", QChar( 968 ), 0, Entity ) );
    items.insert( "omega", CompletionItem( "omega", QChar( 969 ), 0, Entity ) );
    items.insert( "thetasym", CompletionItem( "thetasym", QChar( 977 ), 0, Entity ) );
    items.insert( "upsih", CompletionItem( "upsih", QChar( 978 ), 0, Entity ) );
    items.insert( "piv", CompletionItem( "piv", QChar( 982 ), 0, Entity ) );
    items.insert( "ensp", CompletionItem( "ensp", QChar( 8194 ), 0, Entity ) );
    items.insert( "emsp", CompletionItem( "emsp", QChar( 8195 ), 0, Entity ) );
    items.insert( "thinsp", CompletionItem( "thinsp", QChar( 8201 ), 0, Entity ) );
    items.insert( "zwnj", CompletionItem( "zwnj", QChar( 8204 ), 0, Entity ) );
    items.insert( "zwj", CompletionItem( "zwj", QChar( 8205 ), 0, Entity ) );
    items.insert( "lrm", CompletionItem( "lrm", QChar( 8206 ), 0, Entity ) );
    items.insert( "rlm", CompletionItem( "rlm", QChar( 8207 ), 0, Entity ) );
    items.insert( "ndash", CompletionItem( "ndash", QChar( 8211 ), 0, Entity ) );
    items.insert( "mdash", CompletionItem( "mdash", QChar( 8212 ), 0, Entity ) );
    items.insert( "lsquo", CompletionItem( "lsquo", QChar( 8216 ), 0, Entity ) );
    items.insert( "rsquo", CompletionItem( "rsquo", QChar( 8217 ), 0, Entity ) );
    items.insert( "sbquo", CompletionItem( "sbquo", QChar( 8218 ), 0, Entity ) );
    items.insert( "ldquo", CompletionItem( "ldquo", QChar( 8220 ), 0, Entity ) );
    items.insert( "rdquo", CompletionItem( "rdquo", QChar( 8221 ), 0, Entity ) );
    items.insert( "bdquo", CompletionItem( "bdquo", QChar( 8222 ), 0, Entity ) );
    items.insert( "dagger", CompletionItem( "dagger", QChar( 8224 ), 0, Entity ) );
    items.insert( "Dagger", CompletionItem( "Dagger", QChar( 8225 ), 0, Entity ) );
    items.insert( "bull", CompletionItem( "bull", QChar( 8226 ), 0, Entity ) );
    items.insert( "hellip", CompletionItem( "hellip", QChar( 8230 ), 0, Entity ) );
    items.insert( "permil", CompletionItem( "permil", QChar( 8240 ), 0, Entity ) );
    items.insert( "prime", CompletionItem( "prime", QChar( 8242 ), 0, Entity ) );
    items.insert( "Prime", CompletionItem( "Prime", QChar( 8243 ), 0, Entity ) );
    items.insert( "lsaquo", CompletionItem( "lsaquo", QChar( 8249 ), 0, Entity ) );
    items.insert( "rsaquo", CompletionItem( "rsaquo", QChar( 8250 ), 0, Entity ) );
    items.insert( "oline", CompletionItem( "oline", QChar( 8254 ), 0, Entity ) );
    items.insert( "frasl", CompletionItem( "frasl", QChar( 8260 ), 0, Entity ) );
    items.insert( "euro", CompletionItem( "euro", QChar( 8364 ), 0, Entity ) );
    items.insert( "image", CompletionItem( "image", QChar( 8465 ), 0, Entity ) );
    items.insert( "weierp", CompletionItem( "weierp", QChar( 8472 ), 0, Entity ) );
    items.insert( "real", CompletionItem( "real", QChar( 8476 ), 0, Entity ) );
    items.insert( "trade", CompletionItem( "trade", QChar( 8482 ), 0, Entity ) );
    items.insert( "alefsym", CompletionItem( "alefsym", QChar( 8501 ), 0, Entity ) );
    items.insert( "larr", CompletionItem( "larr", QChar( 8592 ), 0, Entity ) );
    items.insert( "uarr", CompletionItem( "uarr", QChar( 8593 ), 0, Entity ) );
    items.insert( "rarr", CompletionItem( "rarr", QChar( 8594 ), 0, Entity ) );
    items.insert( "darr", CompletionItem( "darr", QChar( 8595 ), 0, Entity ) );
    items.insert( "harr", CompletionItem( "harr", QChar( 8596 ), 0, Entity ) );
    items.insert( "crarr", CompletionItem( "crarr", QChar( 8629 ), 0, Entity ) );
    items.insert( "lArr", CompletionItem( "lArr", QChar( 8656 ), 0, Entity ) );
    items.insert( "uArr", CompletionItem( "uArr", QChar( 8657 ), 0, Entity ) );
    items.insert( "rArr", CompletionItem( "rArr", QChar( 8658 ), 0, Entity ) );
    items.insert( "dArr", CompletionItem( "dArr", QChar( 8659 ), 0, Entity ) );
    items.insert( "hArr", CompletionItem( "hArr", QChar( 8660 ), 0, Entity ) );
    items.insert( "forall", CompletionItem( "forall", QChar( 8704 ), 0, Entity ) );
    items.insert( "part", CompletionItem( "part", QChar( 8706 ), 0, Entity ) );
    items.insert( "exist", CompletionItem( "exist", QChar( 8707 ), 0, Entity ) );
    items.insert( "empty", CompletionItem( "empty", QChar( 8709 ), 0, Entity ) );
    items.insert( "nabla", CompletionItem( "nabla", QChar( 8711 ), 0, Entity ) );
    items.insert( "isin", CompletionItem( "isin", QChar( 8712 ), 0, Entity ) );
    items.insert( "notin", CompletionItem( "notin", QChar( 8713 ), 0, Entity ) );
    items.insert( "ni", CompletionItem( "ni", QChar( 8715 ), 0, Entity ) );
    items.insert( "prod", CompletionItem( "prod", QChar( 8719 ), 0, Entity ) );
    items.insert( "sum", CompletionItem( "sum", QChar( 8721 ), 0, Entity ) );
    items.insert( "minus", CompletionItem( "minus", QChar( 8722 ), 0, Entity ) );
    items.insert( "lowast", CompletionItem( "lowast", QChar( 8727 ), 0, Entity ) );
    items.insert( "radic", CompletionItem( "radic", QChar( 8730 ), 0, Entity ) );
    items.insert( "prop", CompletionItem( "prop", QChar( 8733 ), 0, Entity ) );
    items.insert( "infin", CompletionItem( "infin", QChar( 8734 ), 0, Entity ) );
    items.insert( "ang", CompletionItem( "ang", QChar( 8736 ), 0, Entity ) );
    items.insert( "and", CompletionItem( "and", QChar( 8743 ), 0, Entity ) );
    items.insert( "or", CompletionItem( "or", QChar( 8744 ), 0, Entity ) );
    items.insert( "cap", CompletionItem( "cap", QChar( 8745 ), 0, Entity ) );
    items.insert( "cup", CompletionItem( "cup", QChar( 8746 ), 0, Entity ) );
    items.insert( "int", CompletionItem( "int", QChar( 8747 ), 0, Entity ) );
    items.insert( "there4", CompletionItem( "there4", QChar( 8756 ), 0, Entity ) );
    items.insert( "sim", CompletionItem( "sim", QChar( 8764 ), 0, Entity ) );
    items.insert( "cong", CompletionItem( "cong", QChar( 8773 ), 0, Entity ) );
    items.insert( "asymp", CompletionItem( "asymp", QChar( 8776 ), 0, Entity ) );
    items.insert( "ne", CompletionItem( "ne", QChar( 8800 ), 0, Entity ) );
    items.insert( "equiv", CompletionItem( "equiv", QChar( 8801 ), 0, Entity ) );
    items.insert( "le", CompletionItem( "le", QChar( 8804 ), 0, Entity ) );
    items.insert( "ge", CompletionItem( "ge", QChar( 8805 ), 0, Entity ) );
    items.insert( "sub", CompletionItem( "sub", QChar( 8834 ), 0, Entity ) );
    items.insert( "sup", CompletionItem( "sup", QChar( 8835 ), 0, Entity ) );
    items.insert( "nsub", CompletionItem( "nsub", QChar( 8836 ), 0, Entity ) );
    items.insert( "sube", CompletionItem( "sube", QChar( 8838 ), 0, Entity ) );
    items.insert( "supe", CompletionItem( "supe", QChar( 8839 ), 0, Entity ) );
    items.insert( "oplus", CompletionItem( "oplus", QChar( 8853 ), 0, Entity ) );
    items.insert( "otimes", CompletionItem( "otimes", QChar( 8855 ), 0, Entity ) );
    items.insert( "perp", CompletionItem( "perp", QChar( 8869 ), 0, Entity ) );
    items.insert( "sdot", CompletionItem( "sdot", QChar( 8901 ), 0, Entity ) );
    items.insert( "lceil", CompletionItem( "lceil", QChar( 8968 ), 0, Entity ) );
    items.insert( "rceil", CompletionItem( "rceil", QChar( 8969 ), 0, Entity ) );
    items.insert( "lfloor", CompletionItem( "lfloor", QChar( 8970 ), 0, Entity ) );
    items.insert( "rfloor", CompletionItem( "rfloor", QChar( 8971 ), 0, Entity ) );
    items.insert( "lang", CompletionItem( "lang", QChar( 9001 ), 0, Entity ) );
    items.insert( "rang", CompletionItem( "rang", QChar( 9002 ), 0, Entity ) );
    items.insert( "loz", CompletionItem( "loz", QChar( 9674 ), 0, Entity ) );
    items.insert( "spades", CompletionItem( "spades", QChar( 9824 ), 0, Entity ) );
    items.insert( "clubs", CompletionItem( "clubs", QChar( 9827 ), 0, Entity ) );
    items.insert( "hearts", CompletionItem( "hearts", QChar( 9829 ), 0, Entity ) );
    items.insert( "diams", CompletionItem( "diams", QChar( 9830 ), 0, Entity ) );
    return items.values();
}


QList< SgmlCodeCompletionModel::CompletionItem > SgmlCodeCompletionModel::findAll(KTextEditor::Document* document, const KTextEditor::Range& range) const
{
    DUChainReadLocker lock;
    TopDUContext * tc = 0;
    QHash < QString,  CompletionItem > items;

    tc = DUChain::self()->chainForDocument(document->url());
    if (!tc) return items.values();

    QList< Declaration* > declarations;
    walkChain(tc, declarations);

    foreach(Declaration * d, declarations) {
        if (!d) continue;
        if (d->kind() == Declaration::Type) {
            ElementDeclaration *elementDec = dynamic_cast<ElementDeclaration *>(d);
            if (!elementDec || elementDec->elementType() != ElementDeclarationData::Element) continue;
            QString name = elementDec->name().str();
            items.insert(name, CompletionItem(name, 0, Element));
        }
    }

    return items.values();
}

QList< SgmlCodeCompletionModel::CompletionItem > SgmlCodeCompletionModel::findAttributes(
    KTextEditor::Document* document,
    const KTextEditor::Range& range,
    const QString &element
    ) const
{
    DUChainReadLocker lock;
    DUContext * ctx = 0;
    Declaration *dec = 0;
    ElementDeclaration *elementDec = 0;
    QString name = element;
    QList<Declaration *> decs;
    TopDUContext * tc = 0;
    QHash< QString, CompletionItem > items;
    debug();
    tc = DUChain::self()->chainForDocument(document->url());
    if (!tc) {
        debug() << "No top context";
        return items.values();
    }

    //NO? try the chain then
    if(name.isEmpty()) {
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

        name = dec->identifier().toString();
    }

    decs.append(tc->findDeclarations(Identifier(name.toLower()), SimpleCursor::invalid(), tc));

    foreach(Declaration * d, decs) {
        if (!d || d->kind() != Declaration::Type || !d->internalContext())
            continue;
        elementDec = dynamic_cast<ElementDeclaration *>(d);
        if (!elementDec) continue;
        for (int i = 0; i < elementDec->attributesSize(); i++) {
            IndexedString str = elementDec->attributes()[i];
            items.insert(str.str(), CompletionItem(str.str(), 0, Attribute));
        }
    }

    return items.values();
}


QList< SgmlCodeCompletionModel::CompletionItem > SgmlCodeCompletionModel::findChildElements(
    KTextEditor::Document* document,
    const KTextEditor::Range& range,
    const QString &element
    ) const
{
    DUChainReadLocker lock;
    DUContext * ctx = 0;
    Declaration *dec = 0;
    ElementDeclaration *elementDec = 0;
    QString name = element;
    QList<Declaration *> decs;
    TopDUContext * tc = 0;
    QHash<QString, CompletionItem > items;

    tc = DUChain::self()->chainForDocument(document->url());
    if (!tc) return items.values();

    //NO? try the chain then
    if(name.isEmpty()) {
        ctx = tc->findContextAt(SimpleCursor(range.start()));
        if (!ctx) return items.values();

        dec = ctx->owner();
        if (!dec) return items.values();
        name = dec->identifier().toString();
    }

    decs.append(tc->findDeclarations(Identifier(name.toLower()), SimpleCursor::invalid(), tc));

    foreach(Declaration * dc, decs) {
        if (!dc || dc->kind() != Declaration::Type || !dc->internalContext())
            continue;
        foreach(Declaration * d, dc->internalContext()->localDeclarations()) {
            if (!d || d->kind() != Declaration::Instance) continue;
            ElementDeclaration * elementDec = dynamic_cast<ElementDeclaration *>(d);
            if (!elementDec) continue;
            QString name = elementDec->name().str();
            if (!name.startsWith("#"))
                items.insert(name, CompletionItem(name, 10, Element));
        }
    }

    return items.values();
}

QList< SgmlCodeCompletionModel::CompletionItem > SgmlCodeCompletionModel::findHeadersForDocument(Document* document) const
{
    KMimeType::Ptr mime = KMimeType::mimeType(document->mimeType());
    QList< SgmlCodeCompletionModel::CompletionItem > items;
    
    if(mime->is("application/xhtml+xml") 
        || mime->is("application/docbook+xml")
        || mime->is("application/xml") 
        || mime->is("application/xslt+xml") 
        || mime->is("application/xsd")
        || mime->is("application/wsdl+xml")
        || mime->is("application/x-wsdl")) {
            items.append(CompletionItem("<?xml version=\"1.0\" encoding=\"UTF-8\"?>", 0, Header));
    }
    
    if(mime->is("text/html")) {
        items.append(CompletionItem("<!DOCTYPE html>", 10, Other));
        items.append(CompletionItem("<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 Transitional//EN\" \"http://www.w3.org/TR/html4/loose.dtd\">", 0, Header));
        items.append(CompletionItem("<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01//EN\" \"http://www.w3.org/TR/html4/strict.dtd\">", 0, Header));
        items.append(CompletionItem("<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 Frameset//EN\" \"http://www.w3.org/TR/html4/frameset.dtd\">", 0, Header));
    }
    
    if(mime->is("application/docbook+xml")) {
        items.append(CompletionItem("<!DOCTYPE book>", 10, Other));
        items.append(CompletionItem("<!DOCTYPE book PUBLIC \"-//OASIS//DTD DocBook XML V4.5//EN\" \"http://www.oasis-open.org/docbook/xml/4.5/docbookx.dtd\">", 0, Header));
    }
    
    return items;
}


#include "sgmlcodecompletionmodel.moc"

