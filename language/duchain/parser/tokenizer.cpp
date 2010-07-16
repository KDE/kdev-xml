/*****************************************************************************
 * Copyright (c) 2010 Ruan Strydom <rm3dom@gmail.com>                        *
 *                                                                           *
 * This program is free software; you can redistribute it and/or modify      *
 * it under the terms of the GNU General Public License as published by      *
 * the Free Software Foundation; either version 2 of the License, or         *
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

#include "tokenizer.h"

#include "../language_debug.h"

#include <QtCore/QRegExp>

#include <KDE/KMimeType>

#include "dtdhelper.h"


using namespace Xml;

Tokenizer::Tokenizer(TokenStream* tokenstream, const QString &content)
{
    m_content = content;
    init(tokenstream, m_content.data(), m_content.length());
}

Tokenizer::Tokenizer(TokenStream* tokenstream, const QChar* content, qint64 contentLength)
{
    init(tokenstream, content, contentLength);
}

void Tokenizer::init(TokenStream* tokenstream, const QChar* content, qint64 contentLength) {
    m_tokenStream = tokenstream;
    m_tokenEnd = 0;
    m_tokenBegin = 0;
    m_curpos = 0;
    setDtdHelper(DtdHelper::instanceForName("xml"));
    m_contentData = content;
    m_contentLength = contentLength;
    m_enlc = 0;
    m_processEndline = true;
    m_doctypeRegexp.setPattern(".*<!\\s*doctype\\s+([\\w\\d]*)\\s*(public\\s+([\"'])([^\"']*)[\"'](\\s+[\"']([^\"']*)[\"'])?)?(system\\s*[\"']([^\"']*)[\"'])?.*");
    m_doctypeRegexp.setCaseSensitivity(Qt::CaseInsensitive);
}

Tokenizer::~Tokenizer()
{

}

int Tokenizer::currentState() const
{
    if(m_states.size() > 0)
        return m_states.top().state;
    return 0;
}


void Tokenizer::setDtdHelper(DtdHelper helper)
{
    m_dtdHelper = helper;
}


qint64 Tokenizer::tokenBegin() const {
    return m_tokenBegin;
}

qint64 Tokenizer::tokenEnd() const {
    return m_tokenEnd;
}

const QChar * Tokenizer::readUntill(const QChar *start, const QChar *end, const QString& cond, int flag, const QChar** conds) {
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
                    && ( {if (!condsSet && (*conds=startc) && (condsSet = true)){}true;})
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


const QChar * Tokenizer::readUntillAny(const QChar *start, const QChar *end, const QString& condition, int flag) {
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

const QChar* Tokenizer::readWhileAny(const QChar* start, const QChar* end, const QString& condition, int flag, const QChar** conditionStart) {
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

QString Tokenizer::removeWhites(QString str) const {
    return str.remove(' ').remove('\r').remove('\n').remove('\t');
}

QString Tokenizer::elementName(const QString& token) const
{
    // </ns:element> or < ns:element attrib="blah"> or <!DOCTYPE .. or <?php ..
    //   ^        ^       ^        ^                     ^     ^         ^ ^
    QString name;
    QString seps = "<>/?![]";
    for(int i = 0; i < token.size(); i++) {
        if(token[i].isSpace() || seps.contains(token[i])) {
            if(!name.isEmpty()) break;
            continue;
        }
        name += token[i];
    }
    if(name.isEmpty())
        debug() << "Element name match failed:" << token;
    return name;
}

void Tokenizer::doctype(const QString& token, QString& name, QString& publicId, QString& systemId) const
{
    if (m_doctypeRegexp.exactMatch(token)) {
        name = m_doctypeRegexp.cap(1);
        publicId = m_doctypeRegexp.cap(4);
        systemId = m_doctypeRegexp.cap(6);
        if (systemId.isEmpty())
            systemId = m_doctypeRegexp.cap(8);
    } else {
        debug() << "Doctype match failed!";
    }
}

void Tokenizer::createNewline(int pos)
{
    if (!m_processEndline)
        return;
    if (m_tokenStream) m_tokenStream->locationTable()->newline(pos);
    m_enlc++;
    //debug() << "New line at:" << pos << "# Lines:" << m_enlc;
}
