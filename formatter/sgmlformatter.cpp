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

#include "sgmlformatter.h"
#include "formatter_debug.h"

#include <QtCore/QStack>

#include <interfaces/isourceformatter.h>
#include <interfaces/icore.h>
#include <interfaces/isourceformattercontroller.h>

using namespace Xml;

SgmlFormatter::SgmlFormatter() : Formatter(), m_content(QString()) {
    m_options.insert ( "INDENT", QVariant ( 4 ) );
    m_debugEnabled = false;
    m_contentDataIndex = 0;
    m_lineCount = 0;
}

SgmlFormatter::~SgmlFormatter() {}

void SgmlFormatter::loadStyle ( const QString& content ) {
    if ( content.isNull() )
        return;
    m_options = stringToOptionMap ( content );
}

QString SgmlFormatter::saveStyle() {
    return KDevelop::ISourceFormatter::optionMapToString ( m_options );
}

QString SgmlFormatter::tokenTypeString ( SgmlFormatter::TokenType type ) const {
    switch ( type ) {
    case  Error:
        return "Error";
    case  Processing:
        return "Processing";
    case  ClosedElement:
        return "ClosedElement";
    case  StartElement:
        return "StartElement";
    case  EndElement:
        return "EndElement";
    case  DTD:
        return "DTD";
    case  Comment:
        return "Comment";
    case  Text:
        return "Text";
    case  Other:
        return "Other";
    default:
        return "Undefined";
    };
    return "Undefined";
}

QString SgmlFormatter::indentString ( int depth, int indent ) const {
    if ( depth < 1 || indent < 1 )
        return "";
    int size = depth * indent + 1;
    char s[size];
    memset ( s, ' ', size );
    s[size-1] = '\0';
    return s;
}

/*
    <body background=#FF
          onload="blah">
    body -> uppercase or lower case
    onload -> trimmed and left alligned
*/
QString SgmlFormatter::formatTag(const QString &element, const QString& newLineIndent, const QString& indent) const
{
#define ceq(i, cv) c[i].unicode() == cv
    QString ret;
    QString tag;
    const QChar *c = element.data();
    const QChar *cend = element.data() + element.length();
    int tagNameLength = 0;
    bool hasTagName = false;
    bool lowerCaseTag = m_options.value("CASE").toString() == "LOWER";
    bool upperCaseTag = m_options.value("CASE").toString() == "UPPER";
    while (c < cend) {
        if (c->isSpace() && !hasTagName) {
            c++;
            continue;
        }
        if (ceq(0, '<')) {
            tag+=*c;
            c++;
            continue;
        }
        if (!hasTagName) {
            while (c < cend && !c->isSpace()) {
                if (lowerCaseTag)
                    tag+=c->toLower();
                else if (upperCaseTag)
                    tag+=c->toUpper();
                else
                    tag+=*c;
                tagNameLength++;
                c++;
            }
            hasTagName = true;
            continue;
        }
        tag+=*c;
        c++;
    }
    QString sep = indentString(1, tagNameLength+2);
    QStringList list;
    QStringList l = tag.split('\n');
    for (int i = 0; i < l.size(); i++) {
        if (i == 0)
            list << l[0];
        else
            list << sep + leftTrim(l[i]);
    }

    if (list.size() > 1) {
        for ( int i = 0; i < list.size() - 1; i++ ) {
            if (i == 0)
                ret += newLineIndent + list[i] + '\n';
            else
                ret += indent + list[i] + '\n';
        }
        ret += indent + list.last();
    } else
        ret += newLineIndent + list.first();



    return ret;
}


QString SgmlFormatter::formatText(const QString& text, const QString& indent, int* hasNewLine) const
{
    if (text.size() == 0) {
        *hasNewLine = 0;
        return text;
    }
    QString ret;
    QStringList list;
    *hasNewLine = 1;
    QStringList l = text.split('\n');
    foreach(const QString &s, l) {
        //End lines
        if (s.trimmed().isEmpty())
            list << "";
        else
            list << s;
    }

    //Do we have content
    //<body>blah1\nblah2</body> -> nonEmpty nonEmpty
    //<body>\nblah1\n</body>    -> empty nonEmpty empty
    //<body>blah1</body>        -> nonEmpty
    //<body>\n</body>           -> empty empty
    //<body></body>             -> empty
    if ( list.size() > 0) {
        //End line
        if (list.first().isEmpty() && list.size() > 1)
            ret += '\n';
        else if (list.size() > 1)
            ret += list.first() + '\n';
        else
            ret += list.first();
        for ( int i = 1; i < list.size() - 1; i++ ) {
            if (list[i].isEmpty())
                ret += list[i] + '\n';
            else
                ret += indent + leftTrim(list[i]) + '\n';
        }
        //Not end line
        if ( !list.last().isEmpty() && list.size() > 1) {
            ret += indent + leftTrim(list.last());
            *hasNewLine = 0;
        }
    }
    //Not end line
    if (list.size() < 2)
        *hasNewLine = 0;

    return ret;
}



QString SgmlFormatter::leftTrim(const QString& str) const
{
    int i = 0;
    while (i < str.size() && str[i].isSpace() && ++i) {}
    return str.mid(i, str.size()- i);
}

QStringList SgmlFormatter::formatInlineSource(const QString& text, const KMimeType::Ptr mime) const
{
    KDevelop::ISourceFormatter *formatter = KDevelop::ICore::self()->sourceFormatterController()->formatterForMimeType(mime);
    if (!formatter)
        return text.split('\n');
    QStringList list = text.split("\n");
    for (int i = 0; i < list.size(); i++) {
        list[i] = leftTrim(list[i]);
    }
    QString source = formatter->formatSource(list.join("\n"), mime);
    if (source.length() > 0)
        return source.split('\n');
    return text.split('\n');
}

/*
Start tag:
        <html> blah <body>
        do not break leave alone

        <html> blah
        <body>
        indent body
End tag:
        <html><body> blah </body>
        do not break leave alone
        <html><body> blah
        </body>
        indent body

 */
QString SgmlFormatter::formatSource ( const QString& text, const QString& leftContext, const QString& rightContext ) {
    Q_UNUSED ( rightContext );
    if ( text.size() < 1 )
        return text;

    QString strResult = "";
    QTextStream result ( &strResult );
    TokenType type;
    QString token;
    QString content;
    QString endLine = "\n";
    QStack<QString> elements;
    m_contentDataIndex = 0;
    m_lineCount = 0;
    //Used so that we dont print content again when there is a leftContext
    bool hasLeftContext = leftContext.size();

    bool sources = m_options["SOURCES"].toBool();

    int indent = m_options["INDENT"].toInt();
    if ( indent < 0 || indent > 99 )
        indent = 4;

    //TODO Duplication of bellow
    if ( leftContext.size()  > 0 ) {
        m_content = leftContext;
        m_contentData = leftContext.data();
        m_contentDataLength = leftContext.length();

        while ( m_contentDataIndex <  m_contentDataLength) {
            token = nextToken ((int *)&type).toString();
            switch ( type ) {
            case  StartElement: {
                QString name = elementName(token);
                if (name.isEmpty()) return text;

                //pop to parent
                if (elements.size() > 0)
                    while (elements.size() > 0
                            && (!m_dtdHelper.hasChild(elements.top(), name)
                                || m_dtdHelper.emptyElement(elements.top())))
                        elements.pop();
                if (!m_dtdHelper.emptyElement(name))
                    elements << name;

                //CDATA elements
                if (m_dtdHelper.cdataElement(name)) {
                    QStringRef cdata = readCdataElement(name);
                    if (m_debugEnabled)
                        debug() << "CDATA:" << cdata.toString();
                }
                content = "";
            } break;
            case  EndElement: {
                QString name = elementName(token);
                if (name.isEmpty()) return text;

                //pop to parent
                if (elements.size() > 0 && elements.contains(name)) {
                    while (elements.size() > 0 && elements.top() != name)
                        elements.pop();
                    elements.pop();
                }
                content = "";
            } break;
            case  ClosedElement: {
                QString name = elementName(token);
                if (name.isEmpty()) return text;

                //pop to parent
                if (elements.size() > 0)
                    while (elements.size() > 0
                            && (!m_dtdHelper.hasChild(elements.top(), name)
                                || m_dtdHelper.emptyElement(elements.top())))
                        elements.pop();
                content = "";
            } break;
            case  DTD: {
                QString name;
                QString systemId;
                QString publicId;
                doctype(token, name, publicId, systemId);
                DtdHelper helper = DtdHelper::instance(publicId, systemId, QString(), name, m_mime);
                if (!helper.isNull())
                    m_dtdHelper = helper;
                content = "";
            } break;
            case  Error: {
                debug() << "Error in document at line:" << m_lineCount << "token:" << token;
                return text;
            } break;
            case  Text: {
                content += token;
            } break;
            case Comment: {
                content = "";
            }
            case Other: {
                content = "";
            }
            case Processing: {
                content = "";
            }
            default:
                content = "";
                break;
            };
            if (m_debugEnabled)
                debug() << token << " " << tokenTypeString(type) << " " << elements.size();
        }
    }

    //debug() << leftContext << content << text;

    m_content = text;
    m_contentData = text.data();
    m_contentDataLength = text.length();
    m_contentDataIndex = 0;

    while ( m_contentDataIndex <  m_contentDataLength) {
        token = nextToken ( (int *)&type ).toString();
        switch ( type ) {
        case  Error: {
            debug() << "Error in document at line:" << m_lineCount  << "token:" << token;
            return text;
        } break;
        case  Processing: {
            result << content << token;
            content = "";
        } break;
        case  ClosedElement: {
            QString name = elementName(token);
            if (name.isEmpty()) return text;

            int hasNewLine = 1;
            QString indentStr = indentString(elements.size(), indent);
            QString contentIndentStr = indentString(elements.size(), indent);
            if (hasLeftContext) {
                hasLeftContext = false;
                formatText(content, contentIndentStr, &hasNewLine);
            } else
                result << formatText(content, contentIndentStr, &hasNewLine);
            if (!hasNewLine)
                indentStr = "";
            content = "";

            result << formatTag(token, indentStr, contentIndentStr);

            //pop to parent
            if (elements.size() > 0)
                while (elements.size() > 0
                        && (!m_dtdHelper.hasChild(elements.top(), name)
                            || m_dtdHelper.emptyElement(elements.top())))
                    elements.pop();
        }
        break;
        case  StartElement: {
            QString name = elementName(token);
            if (name.isEmpty()) return text;

            //pop to parent
            if (elements.size() > 0)
                while (elements.size() > 0
                        && (!m_dtdHelper.hasChild(elements.top(), name)
                            || m_dtdHelper.emptyElement(elements.top())))
                    elements.pop();
            if (!m_dtdHelper.emptyElement(name)) {
                elements << name;
            }

            int hasNewLine = 1;
            QString indentStr = indentString(elements.size() - 1, indent);
            QString contentIndentStr = indentString(elements.size(), indent);
            if (hasLeftContext) {
                hasLeftContext = false;
                formatText(content, contentIndentStr, &hasNewLine);
            } else
                result << formatText(content, contentIndentStr, &hasNewLine);
            if (!hasNewLine)
                indentStr = "";
            content = "";

            result << formatTag(token, indentStr, contentIndentStr);

            //CDATA elements
            if (m_dtdHelper.cdataElement(name)) {
                QStringRef cdata = readCdataElement(name);
                if (!cdata.isNull() && !cdata.isEmpty() ) {
                    QString str = cdata.toString();
                    result << cdataHook(token, str, contentIndentStr);
                    if (m_debugEnabled)
                        debug() << "CDATA:" << cdata.toString();
                }
            }
            content = "";
        }
        break;
        case  EndElement: {
            QString name = elementName(token);
            if (name.isEmpty()) return text;

            //pop to parent
            if (elements.size() > 0 && elements.contains(name)) {
                while (elements.size() > 0 && elements.top() != name)
                    elements.pop();
                elements.pop();
            }

            int hasNewLine = 1;
            QString indentStr = indentString(elements.size(), indent);
            QString contentIndentStr = indentString(elements.size()+1, indent);
            if (hasLeftContext) {
                hasLeftContext = false;
                formatText(content, contentIndentStr, &hasNewLine);
            } else
                result << formatText(content, contentIndentStr, &hasNewLine);
            if (!hasNewLine)
                indentStr = "";
            content = "";

            result << formatTag(token, indentStr, contentIndentStr);
        }
        break;
        case  DTD: {
            QString name;
            QString systemId;
            QString publicId;
            doctype(token, name, publicId, systemId);
            DtdHelper helper = DtdHelper::instance(publicId, systemId, QString(), name, m_mime);
            if (!helper.isNull())
                m_dtdHelper = helper;
            result << content << token;
            content = "";
        } break;
        case  Comment: {
            int hasNewLine = 1;
            QString indentStr = indentString(elements.size(), indent);
            QString contentIndentStr = indentString(elements.size(), indent);
            result << formatText(content, contentIndentStr, &hasNewLine);
            if (!hasNewLine)
                indentStr = "";
            content = "";
            result << token;
        } break;
        case  Text: {
            content += token;
        } break;
        case  Other: {

            int hasNewLine = 1;
            QString indentStr = indentString(elements.size(), indent);
            QString contentIndentStr = indentString(elements.size(), indent);
            result << formatText(content, contentIndentStr, &hasNewLine);
            if (!hasNewLine)
                indentStr = "";
            content = "";

            if (sources) {
                if (token.startsWith("<?php") || (token.startsWith("<?") && m_mime->is("application/x-php"))) {
                    QStringList sourceList = formatInlineSource(token, KMimeType::mimeType("application/x-php"));
                    if (sourceList.size() > 1) {
                        for (int i = 0; i < sourceList.length()-1; i++) {
                            if (i == 0)
                                result << indentStr << sourceList[i] << endLine;
                            else
                                result << contentIndentStr << sourceList[i] << endLine;
                        }
                        result << contentIndentStr << sourceList.last();
                    } else
                        result << indentStr << sourceList.first();
                } else
                    result << indentStr << token;
            } else {
                result << indentStr << token;
            }
        } break;
        default:
            return text;
        };
        if (m_debugEnabled)
            debug() << token << " " << tokenTypeString(type) << " " << elements.size();
    }
    if ( content.length() > 0 )
        result << content;
    return strResult;
}

QString SgmlFormatter::compactSource ( const QString& text ) {
    if ( text.size() < 1 )
        return text;

    m_content = text;
    m_contentData = text.data();
    m_contentDataLength = text.length();
    m_contentDataIndex = 0;
    m_lineCount = 0;

    QString strResult = "";
    QTextStream result ( &strResult );

    TokenType type;
    QString token;
    QString content;
    bool inElement = false;
    QString endLine = "\n";

    while ( m_contentDataIndex <  m_contentDataLength) {
        token = nextToken ( (int *)&type ).toString();
        switch ( type ) {
        case  Error:
            debug() << "Error in document at line:" << m_lineCount << "token:" << token;
            return text;
            break;
        case  Processing:
            result << token.trimmed();
            content = "";
            break;
        case  ClosedElement: {
            QStringList list;
            result << content.trimmed();
            content = "";
            list = token.split ( endLine );
            result << list.first().trimmed();
            for ( int i = 1; i < list.size(); i++ )
                result << " " << list[i].trimmed();
            inElement = false;
        }
        break;
        case  StartElement: {
            QStringList list;
            result << content.trimmed();
            content = "";
            list = token.split ( endLine );
            result << list.first().trimmed();
            for ( int i = 1; i < list.size(); i++ )
                result << " " << list[i].trimmed();
            inElement = true;
            QString name = elementName(token);
            if (m_dtdHelper.cdataElement(name)) {
                QStringRef cdata = readCdataElement(name);
                result << cdata.toString();
            }
        }
        break;
        case  EndElement:
            result << content.trimmed() << token;
            content = "";
            break;
        case  DTD: {
            QString name;
            QString systemId;
            QString publicId;
            doctype(token, name, publicId, systemId);
            DtdHelper helper = DtdHelper::instance(publicId, systemId, QString(), name, m_mime);
            if (!helper.isNull())
                m_dtdHelper = helper;
            result << token;
            content = "";
        } break;
        case  Comment:
            result << token;
            content = "";
            break;
        case Other:
            result << content.trimmed() << token;
        case  Text:
            content = token;
            break;
        default:
            debug() << "Error in document at line:" << m_lineCount << "token:" << token;
            return text;
        };
    }
    if ( content.length() > 0 )
        result << content;
    return strResult;
}

QStringRef SgmlFormatter::readCdataElement(const QString& name)
{
    const QChar *c = &m_contentData[m_contentDataIndex];
    const QChar * end;
    const QChar * start = 0;
    qint64 length;
    end = readUntill(c,
                     m_contentData + m_contentDataLength,
                     QString("</%1>").arg(name),
                     IgnoreWhites | IgnoreCase,
                     &start);
    if (end == c)
        return QStringRef();
    length = start - c;
    if((c - m_contentData) + length > m_contentDataLength)
      length = m_contentDataLength - (c - m_contentData);
    QStringRef ref(&m_content, c - m_contentData, length);
    m_contentDataIndex += length;
    return ref;
}


QStringRef SgmlFormatter::nextToken(int* tokenType)
{
#define ceq(i, cv) c[i].unicode() == cv

#define cleq(i, cv) c[i].toLower().unicode() == cv

#define cend m_contentData + m_contentDataLength

#define default_return(T)\
    length = end - c;\
    m_contentDataIndex += length;\
    if(m_contentDataIndex > m_contentDataLength)\
      length -= m_contentDataIndex - m_contentDataLength;\
    *tokenType = T;\
    return QStringRef(&m_content, c - m_contentData, length);

    const QChar * end;
    const QChar * start;
    qint64 length;

    *tokenType = Error;
    if (m_contentDataIndex >= m_contentDataLength)
        return QStringRef(&m_content, 0, 0);
    const QChar *c = &m_contentData[m_contentDataIndex];
    if (!c || c->isNull())
        return QStringRef(&m_content, 0, 0);
    if (ceq(0,'<')) {
        //<!
        if (ceq(1, '!')) {
            if (ceq(2, '>')) {
                end = c + 3;
                default_return(Comment)
            }

            if (ceq(2, '-') && ceq(3, '-')) {
                end = readUntill(c+3, cend, "-->", IgnoreWhites, &start);
                default_return(Comment)
            }

            if (ceq(2, '[')) {
                end = readUntillAny(c+3, cend, "[", IgnoreNone) + 1;
                length = end - c;
                QStringRef ref(&m_content, c - m_contentData, length);
                if (ref.toString().toLower().contains("cdata")) {
                    end = readUntill(c, cend, "]]>", IgnoreWhites, &start);
                    default_return(Text)
                }
                default_return(Text)
            }

            if (cleq(2, 'd') && cleq(3, 'o')) {
                //TODO more complex dtd parsing
                end = readUntillAny(c, cend, ">", IgnoreNone) + 1;
                default_return(DTD)
            }
        }

        //<?
        if (ceq(1, '?')) {
            if (cleq(2, 'x') && cleq(3, 'm') && cleq(4, 'l')) {
                end = readUntill(c, cend, "?>", IgnoreNone, &start);
                default_return(Processing)
            }
            end = readUntill(c, cend, "?>", IgnoreNone, &start);
            default_return(Other)
        }

        //<%
        if (ceq(1, '%')) {
            end = readUntill(c, cend, "%>", IgnoreNone, &start);
            default_return(Other)
        }

        //<
        end = readUntillAny(c+1, cend, "><", IgnoreNone) + 1;
        length = end - c;
        m_contentDataIndex += length;
        if (m_contentDataIndex > m_contentDataLength)
            length -= m_contentDataIndex - m_contentDataLength;
        QStringRef ref(&m_content, c - m_contentData, length);
        if (ref.at(ref.length()-1).unicode() == '<')
            *tokenType = Error;
        else if (ref.at(ref.length()-2).unicode() == '/')
            *tokenType = ClosedElement;
        else if (ref.at(1).unicode() == '/')
            *tokenType = EndElement;
        else
            *tokenType = StartElement;
        return ref;
    }

    //including ]]>
    end = readUntillAny(c, cend, "<", IgnoreNone);
    default_return(Text)
}

const QChar * SgmlFormatter::readUntill(const QChar *start, const QChar *end, const QString& cond, int flag, const QChar** conds) {
    Q_ASSERT(start);
    Q_ASSERT(end);
    QString cnd = cond;
    bool nw = flag & IgnoreWhites, nc =flag & IgnoreCase, condsSet = false;
    if (nw) cnd = removeWhites(cnd);
    int icnd = 0, lcnd = cnd.size();
    *conds = 0;
    const QChar *startc;
    while (start <= end && start) {
        if (nw && start->isSpace()) {
            if (start->unicode() == '\n') createNewline(start - m_contentData);
            ++start;
            continue;
        }
        startc = start;
        while (startc <= end) {
            if (icnd == lcnd) return startc;
            //TODO does not cater for \n
            if (nw && startc->isSpace() && ++startc) continue;
            if ((startc->unicode() == cnd[icnd].unicode()
                    || (nc && startc->toLower().unicode() == cnd[icnd].toLower().unicode()) )
            && ( {if (!condsSet && (*conds=startc) && (condsSet = true)) {}true;})
                && ++startc
                && ++icnd ) continue;
            condsSet = false;
            icnd = 0;
            break;
        }
        if (start->unicode() == '\n') createNewline(start - m_contentData);
        start++;
    }
    return start;
}


const QChar * SgmlFormatter::readUntillAny(const QChar *start, const QChar *end, const QString& condition, int flag) {
    Q_ASSERT(start);
    Q_ASSERT(end);
    while (start < end) {
        if (flag & IgnoreWhites && start->isSpace()) {
            if (start->unicode() == '\n') createNewline(start - m_contentData);
            start++;
            continue;
        }
        //TODO slow IgnoreCase if condition holds
        if (condition.contains(*start)
                || (flag & IgnoreCase && condition.toLower().contains(start->toLower()))) {
            return start;
        }
        if (start->unicode() == '\n') createNewline(start - m_contentData);
        start++;
    }
    return start;
}

const QChar* SgmlFormatter::readWhileAny(const QChar* start,
        const QChar* end,
        const QString& condition,
        int flag,
        const QChar** conditionStart) {
    Q_ASSERT(start);
    Q_ASSERT(end);
    bool conditionStartSet = false;
    while (start < end) {
        if (flag & IgnoreWhites && start->isSpace()) {
            if (start->unicode() == '\n') createNewline(start - m_contentData);
            start++;
            continue;
        }
        //TODO slow IgnoreCase if condition holds
        if (condition.contains(*start)
                || (flag & IgnoreCase && condition.toLower().contains(start->toLower()))) {
            if (conditionStart && !conditionStartSet) {
                *conditionStart = start;
                conditionStartSet = true;
            }
            if (start->unicode() == '\n') createNewline(start - m_contentData);
            start++;
            continue;
        }
        break;
    }
    return start;
}

void SgmlFormatter::createNewline(int pos)
{
    Q_UNUSED(pos);
    m_lineCount++;
}

QString SgmlFormatter::removeWhites(QString str) const {
    return str.remove(' ').remove('\r').remove('\n').remove('\t');
}

QString SgmlFormatter::elementName(const QString& token) const
{
    // </ns:element> or < ns:element attrib="blah"> or <!DOCTYPE .. or <?php ..
    //   ^        ^       ^        ^                     ^     ^         ^ ^
    QString name;
    QString seps = "<>/?![]";
    for (int i = 0; i < token.size(); i++) {
        if (token[i].isSpace() || seps.contains(token[i])) {
            if (!name.isEmpty()) break;
            continue;
        }
        name += token[i];
    }
    if (name.isEmpty())
        debug() << "Element name match failed:" << token;
    return name;
}

void SgmlFormatter::doctype(const QString& token, QString& name, QString& publicId, QString& systemId) const
{
    static QRegExp exp(".*<!\\s*doctype\\s+([\\w\\d]*)\\s*(public\\s+([\"'])([^\"']*)[\"'](\\s+[\"']([^\"']*)[\"'])?)?(system\\s*[\"']([^\"']*)[\"'])?.*",
                       Qt::CaseInsensitive);
    if (exp.exactMatch(token)) {
        name = exp.cap(1);
        publicId = exp.cap(4);
        systemId = exp.cap(6);
        if (systemId.isEmpty())
            systemId = exp.cap(8);
    } else {
        debug() << "Doctype match failed:" << token;
    }
}

QString SgmlFormatter::escapeSource ( const QString& text ) {
    QString strResult;
    QTextStream result ( &strResult );
    QChar c;

    for ( int i = 0; i < text.size(); i++ ) {
        c = text[i];

        if ( ( c < 32 ) && ! ( c == 9 || c == 10 || c == 13 ) ) {
            result << QString ( "&#%1;" ).arg ( c.digitValue() );
            continue;
        }

        if ( c == '&' ) {
            int semiColonIndex = text.mid ( i, 10 ).indexOf ( ';', 0 );
            int nextAmpIndex = text.mid ( i, 10 ).indexOf ( '&', 1 );
            if ( ( semiColonIndex == -1 ) || ( ( nextAmpIndex < semiColonIndex ) && ( nextAmpIndex > 0 ) ) )
                result << "&amp;";
            else {
                result << text.mid ( i, semiColonIndex + 1 );
                i += semiColonIndex;
            }
        } else if ( c == '>' )
            result << "&gt;";
        else if ( c == '<' )
            result << "&lt;";
        else if ( c == '"' )
            result << "&quot;";
        else if ( c == '\'' )
            result << "&#39;";
        else if ( c < 160 )
            result << c;
        else
            result << escapeSequenceForChar ( c );
    }
    result.flush();
    return strResult;
}

QString SgmlFormatter::unescapeSource ( const QString& text ) {
    QString tmpStr;
    QChar tmpChar;
    QChar c;
    QString strResult;
    QTextStream result ( &strResult );
    int maxPos = text.length();
    int curPos = 0;
    bool broke = false;

    while ( curPos < maxPos ) {
        c = text[curPos++];
        if ( c == '&' ) {
            broke = true;
            int tmpPos = curPos;
            QChar nextChar = text[tmpPos];
            if ( nextChar.isLetter() ) {
                while ( tmpPos < maxPos ) {
                    nextChar = text[tmpPos++];
                    if ( !nextChar.isLetter() ) {
                        if ( nextChar == ';' ) {
                            tmpStr = text.mid ( curPos, tmpPos - curPos - 1 );
                            tmpChar = charForEscapeSequence ( tmpStr );
                            if ( !tmpChar.isNull() ) {
                                c = tmpChar;
                                curPos = tmpPos;
                                broke = false;
                            }
                        }
                        break;
                    }
                }
            } else if ( nextChar == '#' ) {
                nextChar = text[tmpPos++];
                while ( tmpPos < maxPos ) {
                    nextChar = text[tmpPos++];
                    if ( !nextChar.isNumber() ) {
                        if ( nextChar == ';' ) {
                            tmpStr = text.mid ( curPos + 1, tmpPos - ( curPos + 1 ) - 1 );
                            c = QChar ( tmpStr.toInt() );
                            curPos = tmpPos;
                            if ( !c.isNull() )
                                broke = false;
                        }
                        break;
                    }
                }
            }
        }
        if ( broke )
            return text;
        result << c;
    }
    result.flush();
    return strResult;
}

QHash<QString, QChar> getEscapeTable() {
    QHash<QString, QChar> charTable;
    charTable.insert ( "quot", QChar ( 34 ) );
    charTable.insert ( "amp", QChar ( 38 ) );
    charTable.insert ( "apos", QChar ( 39 ) );
    charTable.insert ( "lt", QChar ( 60 ) );
    charTable.insert ( "gt", QChar ( 62 ) );
    charTable.insert ( "nbsp", QChar ( 160 ) );
    charTable.insert ( "iexcl", QChar ( 161 ) );
    charTable.insert ( "cent", QChar ( 162 ) );
    charTable.insert ( "pound", QChar ( 163 ) );
    charTable.insert ( "curren", QChar ( 164 ) );
    charTable.insert ( "yen", QChar ( 165 ) );
    charTable.insert ( "brvbar", QChar ( 166 ) );
    charTable.insert ( "sect", QChar ( 167 ) );
    charTable.insert ( "uml", QChar ( 168 ) );
    charTable.insert ( "copy", QChar ( 169 ) );
    charTable.insert ( "ordf", QChar ( 170 ) );
    charTable.insert ( "laquo", QChar ( 171 ) );
    charTable.insert ( "not", QChar ( 172 ) );
    charTable.insert ( "shy", QChar ( 173 ) );
    charTable.insert ( "reg", QChar ( 174 ) );
    charTable.insert ( "macr", QChar ( 175 ) );
    charTable.insert ( "deg", QChar ( 176 ) );
    charTable.insert ( "plusmn", QChar ( 177 ) );
    charTable.insert ( "sup2", QChar ( 178 ) );
    charTable.insert ( "sup3", QChar ( 179 ) );
    charTable.insert ( "acute", QChar ( 180 ) );
    charTable.insert ( "micro", QChar ( 181 ) );
    charTable.insert ( "para", QChar ( 182 ) );
    charTable.insert ( "middot", QChar ( 183 ) );
    charTable.insert ( "cedil", QChar ( 184 ) );
    charTable.insert ( "sup1", QChar ( 185 ) );
    charTable.insert ( "ordm", QChar ( 186 ) );
    charTable.insert ( "raquo", QChar ( 187 ) );
    charTable.insert ( "frac14", QChar ( 188 ) );
    charTable.insert ( "frac12", QChar ( 189 ) );
    charTable.insert ( "frac34", QChar ( 190 ) );
    charTable.insert ( "iquest", QChar ( 191 ) );
    charTable.insert ( "Agrave", QChar ( 192 ) );
    charTable.insert ( "Aacute", QChar ( 193 ) );
    charTable.insert ( "Acirc", QChar ( 194 ) );
    charTable.insert ( "Atilde", QChar ( 195 ) );
    charTable.insert ( "Auml", QChar ( 196 ) );
    charTable.insert ( "Aring", QChar ( 197 ) );
    charTable.insert ( "AElig", QChar ( 198 ) );
    charTable.insert ( "Ccedil", QChar ( 199 ) );
    charTable.insert ( "Egrave", QChar ( 200 ) );
    charTable.insert ( "Eacute", QChar ( 201 ) );
    charTable.insert ( "Ecirc", QChar ( 202 ) );
    charTable.insert ( "Euml", QChar ( 203 ) );
    charTable.insert ( "Igrave", QChar ( 204 ) );
    charTable.insert ( "Iacute", QChar ( 205 ) );
    charTable.insert ( "Icirc", QChar ( 206 ) );
    charTable.insert ( "Iuml", QChar ( 207 ) );
    charTable.insert ( "ETH", QChar ( 208 ) );
    charTable.insert ( "Ntilde", QChar ( 209 ) );
    charTable.insert ( "Ograve", QChar ( 210 ) );
    charTable.insert ( "Oacute", QChar ( 211 ) );
    charTable.insert ( "Ocirc", QChar ( 212 ) );
    charTable.insert ( "Otilde", QChar ( 213 ) );
    charTable.insert ( "Ouml", QChar ( 214 ) );
    charTable.insert ( "times", QChar ( 215 ) );
    charTable.insert ( "Oslash", QChar ( 216 ) );
    charTable.insert ( "Ugrave", QChar ( 217 ) );
    charTable.insert ( "Uacute", QChar ( 218 ) );
    charTable.insert ( "Ucirc", QChar ( 219 ) );
    charTable.insert ( "Uuml", QChar ( 220 ) );
    charTable.insert ( "Yacute", QChar ( 221 ) );
    charTable.insert ( "THORN", QChar ( 222 ) );
    charTable.insert ( "szlig", QChar ( 223 ) );
    charTable.insert ( "agrave", QChar ( 224 ) );
    charTable.insert ( "aacute", QChar ( 225 ) );
    charTable.insert ( "acirc", QChar ( 226 ) );
    charTable.insert ( "atilde", QChar ( 227 ) );
    charTable.insert ( "auml", QChar ( 228 ) );
    charTable.insert ( "aring", QChar ( 229 ) );
    charTable.insert ( "aelig", QChar ( 230 ) );
    charTable.insert ( "ccedil", QChar ( 231 ) );
    charTable.insert ( "egrave", QChar ( 232 ) );
    charTable.insert ( "eacute", QChar ( 233 ) );
    charTable.insert ( "ecirc", QChar ( 234 ) );
    charTable.insert ( "euml", QChar ( 235 ) );
    charTable.insert ( "igrave", QChar ( 236 ) );
    charTable.insert ( "iacute", QChar ( 237 ) );
    charTable.insert ( "icirc", QChar ( 238 ) );
    charTable.insert ( "iuml", QChar ( 239 ) );
    charTable.insert ( "eth", QChar ( 240 ) );
    charTable.insert ( "ntilde", QChar ( 241 ) );
    charTable.insert ( "ograve", QChar ( 242 ) );
    charTable.insert ( "oacute", QChar ( 243 ) );
    charTable.insert ( "ocirc", QChar ( 244 ) );
    charTable.insert ( "otilde", QChar ( 245 ) );
    charTable.insert ( "ouml", QChar ( 246 ) );
    charTable.insert ( "divide", QChar ( 247 ) );
    charTable.insert ( "oslash", QChar ( 248 ) );
    charTable.insert ( "ugrave", QChar ( 249 ) );
    charTable.insert ( "uacute", QChar ( 250 ) );
    charTable.insert ( "ucirc", QChar ( 251 ) );
    charTable.insert ( "uuml", QChar ( 252 ) );
    charTable.insert ( "yacute", QChar ( 253 ) );
    charTable.insert ( "thorn", QChar ( 254 ) );
    charTable.insert ( "yuml", QChar ( 255 ) );
    charTable.insert ( "OElig", QChar ( 338 ) );
    charTable.insert ( "oelig", QChar ( 339 ) );
    charTable.insert ( "Scaron", QChar ( 352 ) );
    charTable.insert ( "scaron", QChar ( 353 ) );
    charTable.insert ( "Yuml", QChar ( 376 ) );
    charTable.insert ( "fnof", QChar ( 402 ) );
    charTable.insert ( "circ", QChar ( 710 ) );
    charTable.insert ( "tilde", QChar ( 732 ) );
    charTable.insert ( "Alpha", QChar ( 913 ) );
    charTable.insert ( "Beta", QChar ( 914 ) );
    charTable.insert ( "Gamma", QChar ( 915 ) );
    charTable.insert ( "Delta", QChar ( 916 ) );
    charTable.insert ( "Epsilon", QChar ( 917 ) );
    charTable.insert ( "Zeta", QChar ( 918 ) );
    charTable.insert ( "Eta", QChar ( 919 ) );
    charTable.insert ( "Theta", QChar ( 920 ) );
    charTable.insert ( "Iota", QChar ( 921 ) );
    charTable.insert ( "Kappa", QChar ( 922 ) );
    charTable.insert ( "Lambda", QChar ( 923 ) );
    charTable.insert ( "Mu", QChar ( 924 ) );
    charTable.insert ( "Nu", QChar ( 925 ) );
    charTable.insert ( "Xi", QChar ( 926 ) );
    charTable.insert ( "Omicron", QChar ( 927 ) );
    charTable.insert ( "Pi", QChar ( 928 ) );
    charTable.insert ( "Rho", QChar ( 929 ) );
    charTable.insert ( "Sigma", QChar ( 931 ) );
    charTable.insert ( "Tau", QChar ( 932 ) );
    charTable.insert ( "Upsilon", QChar ( 933 ) );
    charTable.insert ( "Phi", QChar ( 934 ) );
    charTable.insert ( "Chi", QChar ( 935 ) );
    charTable.insert ( "Psi", QChar ( 936 ) );
    charTable.insert ( "Omega", QChar ( 937 ) );
    charTable.insert ( "alpha", QChar ( 945 ) );
    charTable.insert ( "beta", QChar ( 946 ) );
    charTable.insert ( "gamma", QChar ( 947 ) );
    charTable.insert ( "delta", QChar ( 948 ) );
    charTable.insert ( "epsilon", QChar ( 949 ) );
    charTable.insert ( "zeta", QChar ( 950 ) );
    charTable.insert ( "eta", QChar ( 951 ) );
    charTable.insert ( "theta", QChar ( 952 ) );
    charTable.insert ( "iota", QChar ( 953 ) );
    charTable.insert ( "kappa", QChar ( 954 ) );
    charTable.insert ( "lambda", QChar ( 955 ) );
    charTable.insert ( "mu", QChar ( 956 ) );
    charTable.insert ( "nu", QChar ( 957 ) );
    charTable.insert ( "xi", QChar ( 958 ) );
    charTable.insert ( "omicron", QChar ( 959 ) );
    charTable.insert ( "pi", QChar ( 960 ) );
    charTable.insert ( "rho", QChar ( 961 ) );
    charTable.insert ( "sigmaf", QChar ( 962 ) );
    charTable.insert ( "sigma", QChar ( 963 ) );
    charTable.insert ( "tau", QChar ( 964 ) );
    charTable.insert ( "upsilon", QChar ( 965 ) );
    charTable.insert ( "phi", QChar ( 966 ) );
    charTable.insert ( "chi", QChar ( 967 ) );
    charTable.insert ( "psi", QChar ( 968 ) );
    charTable.insert ( "omega", QChar ( 969 ) );
    charTable.insert ( "thetasym", QChar ( 977 ) );
    charTable.insert ( "upsih", QChar ( 978 ) );
    charTable.insert ( "piv", QChar ( 982 ) );
    charTable.insert ( "ensp", QChar ( 8194 ) );
    charTable.insert ( "emsp", QChar ( 8195 ) );
    charTable.insert ( "thinsp", QChar ( 8201 ) );
    charTable.insert ( "zwnj", QChar ( 8204 ) );
    charTable.insert ( "zwj", QChar ( 8205 ) );
    charTable.insert ( "lrm", QChar ( 8206 ) );
    charTable.insert ( "rlm", QChar ( 8207 ) );
    charTable.insert ( "ndash", QChar ( 8211 ) );
    charTable.insert ( "mdash", QChar ( 8212 ) );
    charTable.insert ( "lsquo", QChar ( 8216 ) );
    charTable.insert ( "rsquo", QChar ( 8217 ) );
    charTable.insert ( "sbquo", QChar ( 8218 ) );
    charTable.insert ( "ldquo", QChar ( 8220 ) );
    charTable.insert ( "rdquo", QChar ( 8221 ) );
    charTable.insert ( "bdquo", QChar ( 8222 ) );
    charTable.insert ( "dagger", QChar ( 8224 ) );
    charTable.insert ( "Dagger", QChar ( 8225 ) );
    charTable.insert ( "bull", QChar ( 8226 ) );
    charTable.insert ( "hellip", QChar ( 8230 ) );
    charTable.insert ( "permil", QChar ( 8240 ) );
    charTable.insert ( "prime", QChar ( 8242 ) );
    charTable.insert ( "Prime", QChar ( 8243 ) );
    charTable.insert ( "lsaquo", QChar ( 8249 ) );
    charTable.insert ( "rsaquo", QChar ( 8250 ) );
    charTable.insert ( "oline", QChar ( 8254 ) );
    charTable.insert ( "frasl", QChar ( 8260 ) );
    charTable.insert ( "euro", QChar ( 8364 ) );
    charTable.insert ( "image", QChar ( 8465 ) );
    charTable.insert ( "weierp", QChar ( 8472 ) );
    charTable.insert ( "real", QChar ( 8476 ) );
    charTable.insert ( "trade", QChar ( 8482 ) );
    charTable.insert ( "alefsym", QChar ( 8501 ) );
    charTable.insert ( "larr", QChar ( 8592 ) );
    charTable.insert ( "uarr", QChar ( 8593 ) );
    charTable.insert ( "rarr", QChar ( 8594 ) );
    charTable.insert ( "darr", QChar ( 8595 ) );
    charTable.insert ( "harr", QChar ( 8596 ) );
    charTable.insert ( "crarr", QChar ( 8629 ) );
    charTable.insert ( "lArr", QChar ( 8656 ) );
    charTable.insert ( "uArr", QChar ( 8657 ) );
    charTable.insert ( "rArr", QChar ( 8658 ) );
    charTable.insert ( "dArr", QChar ( 8659 ) );
    charTable.insert ( "hArr", QChar ( 8660 ) );
    charTable.insert ( "forall", QChar ( 8704 ) );
    charTable.insert ( "part", QChar ( 8706 ) );
    charTable.insert ( "exist", QChar ( 8707 ) );
    charTable.insert ( "empty", QChar ( 8709 ) );
    charTable.insert ( "nabla", QChar ( 8711 ) );
    charTable.insert ( "isin", QChar ( 8712 ) );
    charTable.insert ( "notin", QChar ( 8713 ) );
    charTable.insert ( "ni", QChar ( 8715 ) );
    charTable.insert ( "prod", QChar ( 8719 ) );
    charTable.insert ( "sum", QChar ( 8721 ) );
    charTable.insert ( "minus", QChar ( 8722 ) );
    charTable.insert ( "lowast", QChar ( 8727 ) );
    charTable.insert ( "radic", QChar ( 8730 ) );
    charTable.insert ( "prop", QChar ( 8733 ) );
    charTable.insert ( "infin", QChar ( 8734 ) );
    charTable.insert ( "ang", QChar ( 8736 ) );
    charTable.insert ( "and", QChar ( 8743 ) );
    charTable.insert ( "or", QChar ( 8744 ) );
    charTable.insert ( "cap", QChar ( 8745 ) );
    charTable.insert ( "cup", QChar ( 8746 ) );
    charTable.insert ( "int", QChar ( 8747 ) );
    charTable.insert ( "there4", QChar ( 8756 ) );
    charTable.insert ( "sim", QChar ( 8764 ) );
    charTable.insert ( "cong", QChar ( 8773 ) );
    charTable.insert ( "asymp", QChar ( 8776 ) );
    charTable.insert ( "ne", QChar ( 8800 ) );
    charTable.insert ( "equiv", QChar ( 8801 ) );
    charTable.insert ( "le", QChar ( 8804 ) );
    charTable.insert ( "ge", QChar ( 8805 ) );
    charTable.insert ( "sub", QChar ( 8834 ) );
    charTable.insert ( "sup", QChar ( 8835 ) );
    charTable.insert ( "nsub", QChar ( 8836 ) );
    charTable.insert ( "sube", QChar ( 8838 ) );
    charTable.insert ( "supe", QChar ( 8839 ) );
    charTable.insert ( "oplus", QChar ( 8853 ) );
    charTable.insert ( "otimes", QChar ( 8855 ) );
    charTable.insert ( "perp", QChar ( 8869 ) );
    charTable.insert ( "sdot", QChar ( 8901 ) );
    charTable.insert ( "lceil", QChar ( 8968 ) );
    charTable.insert ( "rceil", QChar ( 8969 ) );
    charTable.insert ( "lfloor", QChar ( 8970 ) );
    charTable.insert ( "rfloor", QChar ( 8971 ) );
    charTable.insert ( "lang", QChar ( 9001 ) );
    charTable.insert ( "rang", QChar ( 9002 ) );
    charTable.insert ( "loz", QChar ( 9674 ) );
    charTable.insert ( "spades", QChar ( 9824 ) );
    charTable.insert ( "clubs", QChar ( 9827 ) );
    charTable.insert ( "hearts", QChar ( 9829 ) );
    charTable.insert ( "diams", QChar ( 9830 ) );
    return charTable;
}

QString SgmlFormatter::escapeSequenceForChar ( const QChar& c ) const {
    static QHash<QChar, QString> t = ( {
        QHash<QString, QChar> a = getEscapeTable();
        QHash<QChar, QString> t;
        QHash<QString, QChar>::iterator it;
        for ( it = a.begin(); it != a.end(); ++it )
            t.insert ( it.value(), it.key() );
        t;
    } );
    if ( t.contains ( c ) )
        return QString ( "&%1;" ).arg ( t[c] );
    return c;
}

QChar SgmlFormatter::charForEscapeSequence ( const QString & s ) const {
    static QHash<QString, QChar> t = getEscapeTable();
    return t[s];
}
