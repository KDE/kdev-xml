/*
Copyright (C) 2010  Ruan Strydom <ruan@jcell.co.za>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef TOKENIZER_H
#define TOKENIZER_H

#include "parserexport.h"

#include <QtCore/QStack>
#include <QtCore/QString>
#include <QtCore/QTextStream>

#include <kdev-pg-token-stream.h>

class QString;

namespace KDevPG {
class TokenStream;
}

namespace Xml {

class Token : public KDevPG::Token {};

class TokenStream : public KDevPG::TokenStreamBase<Token> {};

class IDtdHelper;

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

    enum ReadFlag {IgnoreNone = 0, IgnoreWhites = 1, IgnoreCase = 2};

    /** A pattern similar to the expression .*blah
     *  (but the first occurrence blah; not the last)
     *  @return The character after condition.
     */
    const QChar * readUntill(const QChar *start, const QChar *end, const QString& condition , int flag = 0, const QChar** conditionStart = 0);

    /** A pattern similar to the expression [^blah]*
     *  @return The first occurrence of condition.
     */
    const QChar * readUntillAny(const QChar *start, const QChar *end, const QString& condition, int flag = 0);

    /** A pattern similar to the expression [blah]*
     *  @return The character after condition.
     */
    const QChar * readWhileAny(const QChar *start, const QChar *end, const QString& condition, int flag = 0, const QChar** conditionStart = 0);

    QString removeWhites(QString str) const;

    QString elementName(const QString& token) const;

    void doctype(const QString& token, QString &name, QString &publicId, QString &systemId) const;
    
    /** If the source has a doctype this will return the helper found, else the default XML helper
     *  or the helper set.
     * @return The current DTD helper.
     */
    const IDtdHelper * dtdHelper() {return m_dtdHelper;}
    
    void setDtdHelper(const IDtdHelper * helper);
    
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
    
    const IDtdHelper *m_dtdHelper;
    
    TokenStream* m_tokenStream;

    bool m_processEndline;
private:
    void init(TokenStream* tokenstream, const QChar* content, qint64 contentLength);
    int m_enlc;
};

}

#endif //TOKENIZER_H

// kate: space-indent on; indent-width 4; tab-width 4; replace-tabs on; auto-insert-doxygen on