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

#ifndef TOKENIZER_H
#define TOKENIZER_H

#include "parserexport.h"

#include "dtdhelper.h"

#include <QtCore/QStack>
#include <QtCore/QString>
#include <QtCore/QTextStream>

#include <kdev-pg-token-stream.h>
#include <language/duchain/indexedstring.h>


class QString;

namespace KDevPG {
class TokenStream;
}


namespace Xml {

class Token : public KDevPG::Token {};

class TokenStream : public KDevPG::TokenStreamBase<Token> {};

class DtdHelper;

/** Abstract Tokenizer
 *  @see SgmlTokenizer
 *  @see DTDTokenizer
 */
class KDEVSGMLPARSER_EXPORT Tokenizer {
public:

    virtual ~Tokenizer();

    virtual int nextTokenKind() = 0;

    virtual qint64 tokenBegin() const;

    virtual qint64 tokenEnd() const;

public:
    
    int currentState() const;

    enum ReadFlag {IgnoreNone = 0, IgnoreWhites = 1, IgnoreCase = 2};

    /** A pattern similar to the expression .*blah
     *  (but the first occurrence blah; not the last)
     *  @return The character after condition.
     */
    const QChar * readUntill(const QChar *start, const QChar *end, const QString& condition , int flag, const QChar** conditionStart);

    /** A pattern similar to the expression [^blah]*
     *  @return The first occurrence of condition.
     */
    const QChar * readUntillAny(const QChar *start, const QChar *end, const QString& condition, int flag);

    /** A pattern similar to the expression [blah]*
     *  @return The character after condition.
     */
    const QChar * readWhileAny(const QChar *start, const QChar *end, const QString& condition, int flag, const QChar** conditionStart);

    QString removeWhites(QString str) const;

    QString elementName(const QString& token) const;

    void doctype(const QString& token, QString &name, QString &publicId, QString &systemId) const;
    
    /** If the source has a doctype this will return the helper found, else the default XML helper
     *  or the helper set.
     * @return The current DTD helper.
     */
    DtdHelper dtdHelper() {return m_dtdHelper;}
    
    void setDtdHelper(DtdHelper helper);

    /** Sets the url of the current document.
     *  @see m_document
     */
    void setCurrentDocument(const KDevelop::IndexedString &url) {
        m_document = url;
    }

    /** Returns the url of the current document.
     *  @see m_document
     */
    KDevelop::IndexedString currentDocument() const {
        return m_document;
    }
protected:
    explicit Tokenizer(TokenStream* tokenstream, const QString &content);
    explicit Tokenizer(TokenStream* tokenstream, const QChar* content, qint64 contentLength);
    
    void createNewline(int pos);
    
    struct TokenizerState {
        TokenizerState() : state(0), begin(0) {}
        TokenizerState(int s, const QChar * b) : state(s), begin(b) {}
        int state;
        const QChar *begin;
        QStack<int> tokens;
    };

    /** The content to lex. */
    QString m_content;

    /** The content to lex. */
    const QChar *m_contentData;
    
    /** The content to lex. */
    qint64 m_contentLength;
    
    /** The current position in the content. */
    qint64 m_curpos;

    /** The start of the current token. */
    qint64 m_tokenBegin;

    /** The end of the current token. */
    qint64 m_tokenEnd;

    /** The state the lexer is in. */
    QStack<TokenizerState> m_states;
    
    DtdHelper m_dtdHelper;
    
    TokenStream* m_tokenStream;

    bool m_processEndline;

    /** The current doument being tokenized.
     *  To successfully tokenize SGML we need to know the DTD being used.
     *  We use XMLCatalog to resolve the DTD but if the url is relative,
     *  it must be made absolute.
     */
    KDevelop::IndexedString m_document;
private:
    void init(TokenStream* tokenstream, const QChar* content, qint64 contentLength);
    int m_enlc;
    QRegExp m_doctypeRegexp;
};

}

#endif //TOKENIZER_H

// kate: space-indent on; indent-width 4; tab-width 4; replace-tabs on; auto-insert-doxygen on
