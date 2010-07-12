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
#include "completionvisitor.h"

#include <QtGui/QIcon>

#include <KDE/KTextEditor/View>
#include <KDE/KTextEditor/Document>
#include <KDE/KMimeType>

#include <interfaces/icore.h>
#include <interfaces/isourceformattercontroller.h>
#include <interfaces/isourceformatter.h>


using namespace Xml;

using namespace KDevelop;
using namespace KTextEditor;

SgmlCodeCompletionModel::SgmlCodeCompletionModel(QObject* parent): CodeCompletionModel2(parent) {
    m_depth = 0;
}

SgmlCodeCompletionModel::~SgmlCodeCompletionModel() {}

void SgmlCodeCompletionModel::completionInvoked(KTextEditor::View* view, const KTextEditor::Range& range, KTextEditor::CodeCompletionModel::InvocationType invocationType)
{
    Q_UNUSED(invocationType);
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
    //QRegExp expValue(".*(\\w+\\s*)=(\\s*([\"']\\s*\\w*[\"']?)?(\\w+)?)$");
    QRegExp expValue(".*(\\w+\\s*)=(\\s*[\"']?[^=\"']*)$");
    expValue.exactMatch(text);
    QRegExp expEntity(".*([&]\\w*)$");
    expEntity.exactMatch(text);

    QString esc = expTag.cap(1);
    QString tag = expAtt.cap(2);
    QString att = expValue.cap(2);
    QString entity = expEntity.cap(1);

    debug() << text << att;

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

    debug() << "# attribute" << visitor.attributePrettyName();
    debug() << "# element" << visitor.elementPrettyName();
    debug() << "# context" << visitor.contextPrettyName();


    if (text.trimmed().isEmpty()) {
        m_items.append(findHeadersForDocument(view->document()));
    }

    if (esc.endsWith('!') && tag.isEmpty()) {
        m_items.append(CompletionItem::Ptr(new CompletionItem("DOCTYPE", 10, CompletionItem::Other)));
        m_items.append(CompletionItem::Ptr(new CompletionItem("ELEMENT", 10, CompletionItem::Other)));
        m_items.append(CompletionItem::Ptr(new CompletionItem("ENTITY", 10, CompletionItem::Other)));
        m_items.append(CompletionItem::Ptr(new CompletionItem("ATTLIST", 10, CompletionItem::Other)));
        m_items.append(CompletionItem::Ptr(new CompletionItem("NOTATION", 10, CompletionItem::Other)));
        m_items.append(CompletionItem::Ptr(new CompletionItem("[CDATA[", 0, CompletionItem::Other)));
        m_items.append(CompletionItem::Ptr(new CompletionItem("[PCDATA[", 0, CompletionItem::Other)));
        setRowCount(m_items.size());
        return;
    }

    if (!entity.isEmpty()) {
        m_items.append(findAllEntities(view->document(), range));
        setRowCount(m_items.size());
        return;
    }

    //Complete enumerations
    if (visitor.attribute && !att.isEmpty()) {
        m_items.append(findEnumerations(view->document(), range, visitor.elementPrettyName(), visitor.attributePrettyName()));
    }
    //Complete attributes
    else if (visitor.element && !tag.isEmpty()) {
        // get full element to make sure we filter _all_ attributes, including those following current cursor position
        Q_ASSERT(range.onSingleLine());
        QString line = view->document()->line(range.end().line());
        bool looksGood = false;
        ///TODO: multiline tags
        for ( int i = range.end().column(); i < line.length(); ++i ) {
            if (line[i] == '>') {
                line.resize(i + 1);
                looksGood = true;
                break;
            } else if (line[i] == '<') {
                // apparently no proper tag
                break;
            }
        }
        if (looksGood) {
            for ( int i = range.end().column(); i > 0; --i ) {
                if (line[i] == '<') {
                    line = line.mid(i);
                    break;
                } else if (line[i] == '>') {
                    // apparently no proper tag
                    looksGood = false;
                    break;
                }
            }
            if (looksGood ) {
                // ok, found a tag :)
                Q_ASSERT(line.startsWith('<'));
                Q_ASSERT(line.endsWith('>'));
                // remove > to stay in element
                line.chop(1);
                session.setContents(line);
                start = 0;
                if (session.parse(&start)) {
                    visitor.visitNode(start);
                    Q_ASSERT(visitor.element);
                }
            }
        }
        QList< CompletionItem::Ptr > items = findAttributes(view->document(), range, visitor.elementPrettyName(), visitor.attributeNsPrefix());
        QStringList existingAttributes;
        if (visitor.element->attributesSequence) {
        for ( int i = 0; i < visitor.element->attributesSequence->count(); ++i ) {
                AttributeAst *att = visitor.element->attributesSequence->at(i)->element;
                if (att && att->name) {
                QString prettyName = editor.parseSession()->symbol(att->name);
                    if (att->ns)
                        prettyName = QString("%1:%2").arg(editor.parseSession()->symbol(att->ns),editor.parseSession()->symbol(att->name));
                    existingAttributes << prettyName;
                }
            }
        }
        foreach ( const CompletionItem::Ptr& item, items ) {
            if (existingAttributes.contains(item->getName())) {
                continue;
            }
            m_items << item;
        }
    }
    //Complete tags
    else if (visitor.context) {
        if ((esc.endsWith("/") || esc.endsWith(">") || esc.startsWith("</")) && !esc.endsWith("/>"))
            m_items.append(CompletionItem::Ptr(new CompletionItem(QString("/%1").arg(visitor.contextPrettyName()), 10, CompletionItem::Element)));
        QHash<QString, CompletionItem::Ptr> items;
        QList<CompletionItem::Ptr> list = findChildElements(view->document(), range, visitor.contextPrettyName(), visitor.elementNsPrefix());
        foreach(CompletionItem::Ptr i, list) {
            items.insert(i->getName(), i);
        }
        list = findAll(view->document(), range);
        foreach(CompletionItem::Ptr i, list) {
            if (!items.contains(i->getName())) {
                items.insert(i->getName(), i);
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
    const static QString seps = "/\\!<>= \r\n\t[]()'\";&";
    KTextEditor::Cursor start = position;
    KTextEditor::Cursor end = position;
    QString text = view->document()->line(position.line());
    if (!text.isEmpty()) {
        if (start.column() >= text.length() && !seps.contains(text.at(text.length()-1))) {
            start.setColumn(text.length() - 1);
        }
        if (start.column() < text.length()) {
            while (start.column() > 0 && !seps.contains(text.at(start.column()))) {
                start.setColumn(start.column()-1);
                if (seps.contains(text.at(start.column()))) {
                    start.setColumn(start.column()+1);
                    break;
                }
            }
        }
    }
    while (end.column() < text.length() - 1 && !seps.contains(text.at(end.column()))) {
        end.setColumn(end.column()+1);
        if (seps.contains(text.at(end.column()))) {
            end.setColumn(end.column());
            break;
        }
    }
    return Range(start, end);
}

QVariant SgmlCodeCompletionModel::data(const QModelIndex& index, int role) const
{
    if (index.parent().isValid())
        return QVariant();
    if (m_items.count() <= index.row())
        return QVariant();

    if (role == Qt::DisplayRole && index.column() == CodeCompletionModel::Name)
        return QVariant(m_items.at(index.row())->getName());

    if (role == Qt::DisplayRole && index.column() == CodeCompletionModel::Postfix)
        return QVariant(m_items.at(index.row())->getInfo());

    if (role == MatchQuality)
        return QVariant(m_items.at(index.row())->getMatchLevel());

    if (role == InheritanceDepth)
        return QVariant(10 - m_items.at(index.row())->getMatchLevel() + 1);

    if (role == Qt::DecorationRole && index.column() == CodeCompletionModel::Icon)
        return m_items.at(index.row())->getIcon();

    return QVariant();
}


QString SgmlCodeCompletionModel::formatSource(Document* document, const QString& code, const Cursor& pos) const
{
    KMimeType::Ptr mime = KMimeType::mimeType(document->mimeType());
    ISourceFormatter * formatter = ICore::self()->sourceFormatterController()->formatterForMimeType(mime);
    if (formatter) {
        QString leftCtx;
        for ( int i = 0; i <= pos.line(); ++i ) {
            if ( i < pos.line() ) {
                leftCtx += document->line(i) + '\n';
            } else {
                leftCtx += document->line(i).left(pos.column());
            }
        }
        return formatter->formatSource(code, mime, leftCtx);
    }
    return code;
}

QString SgmlCodeCompletionModel::formatItem(Document* document, const QString& str, CompletionItem::CompletionItemType type) const
{
#define CASESENSITIVE 0
#define LOWERCASE 1
#define UPPERCASE 1

    if (type != CompletionItem::Element)
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
    static QString seps = "</!>=\"\'&";
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
    CompletionItem::Ptr item = m_items.at(index.row());

    //NOTE: CustomCompletionItem is not used or tested but added for future use
    if (item->getType() == CompletionItem::Custom) {
        CustomCompletionItem *custom = dynamic_cast<CustomCompletionItem *>(item.data());
        if (custom)
            custom->complete(document, word);
        return;
    }

    QString text = formatItem(document, item->getName(), item->getType());
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
        text = QString("</%1>").arg(text);
        if (trimmedLine.isEmpty()) {
            range.start().setColumn(0);
        }
        text = formatSource(document, text, word.start());
        document->replaceText(range, text);
        foreach(View * v, document->views()) {
            if (v->isActiveView())
                v->setCursorPosition(range.start());
        }
        return;
    }

    Range range = word;

    //Attributes
    if (item->getType() == CompletionItem::Attribute) {
        // overwrite existing attribute to the left
        Cursor pos = range.start();
        while ( pos.column() > 1 && !document->character(pos - Cursor(0, 1)).isSpace() && document->character(pos - Cursor(0, 1)) != ':') {
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
    } else if (item->getType() == CompletionItem::Header) {
        const QString line = document->line(range.start().line());
        bool haveOpenTag = false;
        // overwrite existing tag to the left
        for (int i = range.start().column() - 1; i >= 0; --i) {
            if (line[i] == '>') {
                break;
            } else if (line[i] == '<') {
                // we have an open tag, overwrite it.
                range.start().setColumn(i);
                haveOpenTag = true;
                break;
            }
        }
        // overwrite existing tag to the right
        if (haveOpenTag) {
            for (int i = range.end().column(); i < line.length(); ++i) {
                if (line[i] == '<') {
                    break;
                } else if (line[i] == '>') {
                    // we have an open tag, overwrite it.
                    range.end().setColumn(i + 1);
                    break;
                }
            }
        }
    } else if (item->getType() == CompletionItem::Entity) {
        range = growRangeRight(document, range, ";");
        range = growRangeLeft(document, range, "&");
        text = QString("&%1;").arg(text);
        document->replaceText(range, text);
        return;
    } else if (item->getType() == CompletionItem::Element) {
        range = growRangeLeft(document, range, "<");
        if (!text.startsWith('<')) {
            text.prepend('<');
        }
        if (!text.endsWith('>')) {
            if (item->empty()) {
                text.append('/');
            }
            text.append('>');
        }
        if (trimmedLine.isEmpty()) {
            range.start().setColumn(0);
        }
    }

    text = formatSource(document, text, range.start());
    document->replaceText(range, text);

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
    Q_UNUSED(view);
    Q_UNUSED(range);
    Q_UNUSED(currentCompletion);
    return false;
}

bool SgmlCodeCompletionModel::shouldStartCompletion(View* view, const QString& insertedText, bool userInsertion, const KTextEditor::Cursor& position)
{
    Q_UNUSED(insertedText);
    Q_UNUSED(userInsertion);
    return !getSeperator(view->document(), position).isNull();
}




#include "sgmlcodecompletionmodel.moc"

