--  This file is part of KDevelop
--
--  Copyright 2010 Ruan Strydom <ruan@jcell.co.za>
--
--
--  This library is free software; you can redistribute it and/or
--  modify it under the terms of the GNU Lesser General Public
--  License as published by the Free Software Foundation; either
--  version 2 of the License, or (at your option) any later version.
--
--  This library is distributed in the hope that it will be useful,
--  but WITHOUT ANY WARRANTY; without even the implied warranty of
--  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
--  Lesser General Public License for more details.
--
--  You should have received a copy of the GNU Lesser General Public
--  License along with this library; if not, write to the Free Software
--  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
--

[:
#include "dtdhelper.h"

namespace KDevelop
{
    class DUContext;
}

// needed by debug-visitor, TODO: move that to debug-visitor when kdev-pg-qt 1.0 gets released
#include "tokenizer.h"
:]

%parser_declaration_header "QtCore/QString"
%parser_declaration_header "KDE/KMimeType"
%parser_declaration_header "language/interfaces/iproblem.h"
%parser_declaration_header "QtCore/QList"
%parser_declaration_header "tokenizer.h"
%parser_bits_header "KDE/KDebug"
%parser_bits_header "sgmltokenizer.h"
%parser_bits_header "dtdtokenizer.h"


------------------------------------------------------------
-- Export macro to use the parser in a shared lib
------------------------------------------------------------
%export_macro "KDEVSGMLPARSER_EXPORT"
%export_macro_header "parserexport.h"


------------------------------------------------------------
-- Enumeration types for additional AST members,
-- in the global "Xml" namespace
------------------------------------------------------------
%namespace
[:
    class Tokenizer;
:]

%token_stream TokenStream ;;

------------------------------------------------------------
-- Parser class members
------------------------------------------------------------

%parserclass (public declaration)
[:
  /**
   * Transform the raw input into tokens.
   * When this method returns, the parser's token stream has been filled
   * and any parse*() method can be called.
   */
  void tokenize(const QString& contents);

  enum ProblemType {
      Error,
      Warning,
      Info
  };
  void reportProblem( Parser::ProblemType type, const QString& message );
  QList<KDevelop::ProblemPointer> problems() {
      return m_problems;
  }
  QString tokenText(qint64 begin, qint64 end) const;
  void setDebug(bool debug);
  void setCurrentDocument(KDevelop::IndexedString url);
  ///Tells the parser to use DTD or SGML
  ///If this is not set it will guess the mime using the document URL.
  void setMime(KMimeType::Ptr mime);
:]

%parserclass (private declaration)
[:
    QString m_contents;
    bool m_debug;
    KDevelop::IndexedString m_currentDocument;
    QList<KDevelop::ProblemPointer> m_problems;
    DtdHelper m_dtdHelper;
    QString tagName(const ElementTagAst *ast) const;
    QString tagName(const ElementCloseTagAst *ast) const;
    QStack<ElementTagAst *> m_stack;
    KMimeType::Ptr m_mime;
    bool LAChoice(int &i);
:]



-----------------------------------------------------------
-- List of defined tokens
-----------------------------------------------------------

---------------
-- HTML/XML TOKENS
---------------

%token TEXT ("text"),
       COMMENT ("<!-- -->"),
       LT ("<"),
       GT (">"),
       CLOSE("</ />"),
       --LTFS ("</"),
       --FSGT ("/>"),
       EQUAL ("="),
       COLON (":"),
       CDATA ("<![CDATA[ ]>"),       --Treated seperately because of <![ in DTD
       PCDATA ("<![PCDATA[ ]>"),     --Treated seperately because of <![ in DTD
       PROC ("<?xml ?>"),
       PHP ("<?php ?>"),             --Not Used
       SRC ("<% %>"),                --Not Used <% TEXT %> ie: JSP
       WHITE ("white");;

---------------
-- DTD TOKENS
---------------

%token DOCTYPE("<!DOCTYPE"),
       ELEMENT("<!ELEMENT"),
       ATTLIST("<!ATTLIST"),
       ENTITY("<!ENTITY"),
       NOTATION("<!NOTATION"),
       PUBLIC("PUBLIC"),
       SYSTEM("SYSTEM"),
       --ANY("ANY"),
       --IMPLIED("IMPLIED"),
       --REQUIRED("REQUIRED"),
       FIXED("FIXED"),
       --EMPTY("EMPTY"),
       COND_START("<!["),               --TODO Not used - DTD Condition
       COND_END("]>"),                 --TODO Not used - DTD Condition
       OBRACKET ("["),
       CBRACKET ("]"),
       OPAREN ("("),
       CPAREN (")"),
       VBAR("|"),
       PLUS("+"),
       OPT("O"),                        --Optional tag
       HYPHEN("-"),                     --Required tag
       ASTERISK("*"),
       NUMBER("#"),
       COMMA(","),
       PERCENT("%"),
       SEMICOLON(";"),
       AMP("&"),
       QUOTE ("\"'"),
       QUESTION("?");;

-- data types
-- %token NDATA("NDATA"),
--        --CDATA("CDATA"),
--        --PCDATA("PCDATA"),
--        SDATA("SDATA"),
--      NMTOKEN("NMTOKEN"),
--        IDREFS("IDREFS"),
--        IDREF("IDREF"),
--        NUMBER("NUMBER"),
--        NAME("NAME"),
--        ID("ID");;

-- tokenizer error
%token ERROR ("error") ;;

-- token that makes the parser fail in any case:
%token INVALID ("invalid token") ;;


---------------------------------------------------------------------------------
-- XML etc grammar starts here.
-- NOTE
-- The parser do not create a tree at the moment the logic was that DTD and the mime
-- defines how tag/elements are opened and closed ?
-- The builder can easily push pop the ast nodes based on the mime and/or included DTD ?
---------------------------------------------------------------------------------

    (#elements=element)*
->  start ;;


    --TODO how to get rid of element
    element=elementCloseTag
    | element=elementText
    | element=elementPCDATA
    | element=elementCDATA
    | element=elementPHP --Not Used
    | element=elementTag
    | element=elementSource  --Not Used
    | element=elementProcessing
    | element=elementComment
    | element=dtdDoctype
    | element=dtdEntity
    | element=dtdAttlist
    | element=dtdElement
    | element=dtdCondition
    | element=dtdEntityInclude
    | element=dtdNotation
    | element=error  --Not used
    | maybeWhites --needed for dtdtokenizer
->  element [
        member variable element: AstNode*;
    ];;

    WHITE+ | 0
->  maybeWhites ;;

    WHITE+
->  whites ;;

    TEXT+ | 0
->  maybeText ;;


    TEXT
->  text ;;

    TEXT
->  identifier ;;

    TEXT
->  elementText ;;


    --NOTE safe but irritating FIRST/FIRST conflict
    (?[: LA(2).kind == Token_COLON :] (ns=identifier COLON (name=identifier | 0 [: reportProblem(Warning, "Expected attribute name"); :]))
        | (name=identifier)
    ) maybeWhites
    (
        EQUAL maybeWhites
        (
            value=text
            | 0 [: reportProblem(Warning, "Missing attribute value"); :]
        )
        | 0
    )
->  attribute ;;


    --NOTE safe but irritating FIRST/FIRST conflict
    --TODO Supported tags marked with *:
    --  <sgml>                 * root not a problem
    --      <child1>
    --          blah, blah
    --          <child1.1>     *supported using dtdHelper not a problem
    --      <child2>
    --          <child2.1>     *supported using dtdHelper not a problem
    --              <child2.2> Problem as DTD will specify that child2.2 belongs actualy to child2
    --      </child2>          * will correctly pop to child2

    topen=LT maybeWhites
    (?  [: LA(2).kind == Token_COLON :] (ns=identifier COLON (name=identifier | 0 [: reportProblem(Error, "Expected element name"); :] ))
        | (name=identifier)
        | 0 [: reportProblem(Error, "Expected element name"); :]
    ) maybeWhites
    (#attributes=attribute maybeWhites)*
    (
        tclose=CLOSE [: while(!m_stack.empty() && !m_dtdHelper.hasChild(tagName(m_stack.top()), tagName(*yynode))) m_stack.pop(); :]
        | tclose=GT 
            [: 
                //TODO still broken not working as should, m_stack is fine but the children is still added to the wrong element
                bool thisIsNotYourChildThief = false;
                while(!m_stack.empty() 
                    && m_dtdHelper.closeOptional(tagName(m_stack.top())) 
                    && !m_dtdHelper.hasChild(tagName(m_stack.top()), tagName(*yynode))
                    && (thisIsNotYourChildThief = true))
                    m_stack.pop();

                m_stack.push(*yynode);
            :] ([: if(!m_stack.empty() && m_stack.top() != *yynode) break; :] #children=element [: if(!m_stack.empty() && m_stack.top() != *yynode) break; :])*
        | 0 [: reportProblem(Error, "Unclosed element"); :]
    )
->  elementTag ;;


    topen=CLOSE maybeWhites
    (?  [: LA(2).kind == Token_COLON :] (ns=identifier COLON (name=identifier | 0 [: reportProblem(Error, "Expected element name"); :] ))
        | (name=identifier)
        | 0 [: reportProblem(Error, "Expected element name"); :]
    ) maybeWhites
    (
        tclose=GT [:
                    QString tagNameStr = tagName(*yynode).toLower();
                    if(m_dtdHelper.emptyElement(tagNameStr))
                        reportProblem(Error, "Empty element: no content/close tag expected");
                    else {
                        while(!m_stack.empty() && tagName(m_stack.top()).toLower() != tagNameStr) m_stack.pop();
                        if(m_stack.empty())
                            reportProblem(Error, "Close tag without start");
                        if(!m_stack.empty() && tagName(m_stack.top()).toLower() == tagNameStr)  m_stack.pop();
                    }
                  :]
        | 0 [: reportProblem(Error, "Unclosed element"); :]
    )
->  elementCloseTag ;;


    CDATA
->  elementCDATA ;;


    PCDATA
->  elementPCDATA ;;


    PHP
->  elementPHP ;;


    SRC
->  elementSource ;;


    PROC
->  elementProcessing ;;


    COMMENT
->  elementComment ;;

    ERROR [: reportProblem( Error, "Syntax error" ); :]
->  error ;;

---------------------------------------------------------------------------------
-- DTD grammar starts here.
-- NOTE
-- The tokenizer skips over [ ] sections
---------------------------------------------------------------------------------

    QUOTE | 0
->  dtdMaybeQuote ;;


    (VBAR | AMP) maybeWhites 
->  dtdChoiceSep ;;


    (COMMA | AMP) maybeWhites
->  dtdSequenceSep ;;


    dtdEntityInclude [: reportProblem(Warning, "Entity reference can not be resolved"); :]
->  dtdUnknownEntity ;;


    NUMBER type=text maybeWhites
    | (name=text | dtdUnknownEntity) maybeWhites
->  dtdEnumItem ;;

    NUMBER type=text maybeWhites
    | (name=text | dtdUnknownEntity) maybeWhites
-> dtdElementIncludeItem ;;

    (
        NUMBER type=text
        | name=identifier 
        | dtdUnknownEntity
        | ?[:{
            int i=1;
            bool isChoice = false;
            while(++i < 9999 
                && LA(i).kind != Token_CPAREN 
                && LA(i).kind != Token_COMMA
                && ({if(LA(i).kind == Token_OPAREN && LAChoice(i)); true;})
                && LA(i).kind != Token_EOF)
                if(LA(i).kind == Token_VBAR){ isChoice = true; break;}
                isChoice;
        }:] copen=OPAREN maybeWhites #choice=dtdElementList @ dtdChoiceSep maybeWhites cclose=CPAREN
        | copen=OPAREN maybeWhites maybeWhites #sequence=dtdElementList @ dtdSequenceSep maybeWhites cclose=CPAREN
    )
    ( cardinality=QUESTION | cardinality=ASTERISK | cardinality=PLUS | 0 )
    maybeWhites
->  dtdElementList ;;


    name=identifier whites
    (
        OPAREN maybeWhites #enum=dtdEnumItem @ dtdChoiceSep maybeWhites CPAREN maybeWhites
        ( 
            NUMBER useage=text maybeWhites (QUOTE (defaultValue=text | 0) QUOTE | 0)
            | QUOTE (defaultValue=text | 0) QUOTE
            | defaultValue=text
            | dtdUnknownEntity (QUOTE (defaultValue=text | 0) QUOTE | 0)
        )
        | type=text maybeWhites 
        ( 
            NUMBER useage=text maybeWhites (QUOTE (defaultValue=text | 0) QUOTE | 0)
            | QUOTE (defaultValue=text | 0) QUOTE
            | defaultValue=text
            | dtdUnknownEntity (QUOTE (defaultValue=text | 0) QUOTE | 0)
            | OPAREN maybeWhites #enum=dtdEnumItem @ dtdChoiceSep maybeWhites CPAREN maybeWhites
            ( 
                NUMBER useage=text maybeWhites (QUOTE (defaultValue=text | 0) QUOTE | 0)
                | QUOTE (defaultValue=text | 0) QUOTE
                | defaultValue=text
                | dtdUnknownEntity (QUOTE (defaultValue=text | 0) QUOTE | 0)
            )
        )
        | dtdUnknownEntity
    ) 
->  dtdAtt ;;


    topen=ATTLIST whites
    (
        (name=identifier | dtdUnknownEntity)
        | OPAREN maybeWhites #elements=dtdEnumItem @ dtdChoiceSep maybeWhites CPAREN
        | 0 [: reportProblem(Error, "Expected element name or list"); :]
    )
    whites
    (#attributes=dtdAtt maybeWhites)*
    ( tclose=GT | 0 [: reportProblem(Error, "Unclosed element"); :] )
->  dtdAttlist ;;


    topen=ENTITY whites
    (
        (persent=PERCENT maybeWhites | 0 [: (*yynode)->persent=0; :] ) name=identifier  maybeWhites
        (
            PUBLIC maybeWhites QUOTE publicId=text QUOTE maybeWhites (SYSTEM maybeWhites | 0) (QUOTE systemId=text QUOTE| 0)
            | SYSTEM maybeWhites (QUOTE systemId=text QUOTE | 0)
            | type=text maybeWhites (QUOTE (value=text | 0) QUOTE | 0)
            | QUOTE (value=text | 0) QUOTE
            | dtdUnknownEntity (QUOTE (value=text | 0) QUOTE| 0)
            | 0 [: reportProblem(Error, "Expected one of PUBLIC, SYSTEM or value"); :]
        )
        | 0 [: reportProblem(Error, "Expected entity name"); :]
    )
    maybeWhites
    ( tclose=GT | 0 [: reportProblem(Error, "Unclosed element"); :] )
->  dtdEntity ;;


    topen=NOTATION maybeWhites
    name=identifier maybeWhites
    (
        PUBLIC maybeWhites QUOTE publicId=text QUOTE maybeWhites (SYSTEM maybeWhites | 0) (QUOTE systemId=text QUOTE| 0)
        | SYSTEM maybeWhites (QUOTE systemId=text QUOTE | 0)
        | 0 [: reportProblem(Error, "Expected PUBLIC or SYSTEM"); :]
    )
    maybeWhites
    ( tclose=GT | 0 [: reportProblem(Error, "Unclosed element"); :] )
->  dtdNotation ;;

    PERCENT name=identifier ( SEMICOLON | 0 )
->  dtdEntityInclude ;;

    topen=ELEMENT whites
    (
        (name=identifier | dtdUnknownEntity)
        | OPAREN maybeWhites #elements=dtdEnumItem @ dtdChoiceSep maybeWhites CPAREN
        | 0 [: reportProblem(Error, "Expected element name or list"); :]
    )
    maybeWhites
    ((openTag=HYPHEN | openTag=OPT) whites (closeTag=HYPHEN | closeTag=OPT) | dtdUnknownEntity | 0 [: (*yynode)->closeTag=0; :])
    maybeWhites
    (
        content=text 
        | dtdUnknownEntity
        | ?[:{
            int i=1;
            bool isChoice = false;
            while(++i < 9999 
                && LA(i).kind != Token_CPAREN 
                && LA(i).kind != Token_COMMA
                && ({if(LA(i).kind == Token_OPAREN && LAChoice(i)); true;})
                && LA(i).kind != Token_EOF)
                if(LA(i).kind == Token_VBAR){ isChoice = true; break;}
                isChoice;
        }:] copen=OPAREN maybeWhites #choice=dtdElementList @ dtdChoiceSep maybeWhites cclose=CPAREN
        | copen=OPAREN maybeWhites maybeWhites #sequence=dtdElementList @ dtdSequenceSep maybeWhites cclose=CPAREN
    )
    ( cardinality=QUESTION | cardinality=ASTERISK | cardinality=PLUS | 0 )
    maybeWhites
    (
        PLUS OPAREN maybeWhites #include=dtdElementIncludeItem @ dtdChoiceSep maybeWhites CPAREN
        | HYPHEN OPAREN maybeWhites #exclude=dtdEnumItem @ dtdChoiceSep maybeWhites CPAREN
        | dtdUnknownEntity
        | 0
    )
    maybeWhites
    ( tclose=GT | 0 [: reportProblem(Error, "Unclosed element"); :] )
->  dtdElement ;;



    child=dtdAttlist
    | child=dtdEntity
    | child=dtdElement
    | child=dtdCondition
    | child=dtdEntityInclude
    | child=dtdNotation
    | child=error --Not used
->  dtdChild [
        member variable child: AstNode*;
    ];;

    topen=COND_START
    maybeWhites (condition=text | dtdUnknownEntity) maybeWhites copen=OBRACKET maybeWhites (#children=dtdChild maybeWhites)* maybeWhites cclose=CBRACKET
    tclose=COND_END
->  dtdCondition ;;

    topen=DOCTYPE maybeWhites
    name=identifier maybeWhites
    (PUBLIC maybeWhites QUOTE publicId=maybeText QUOTE maybeWhites (SYSTEM maybeWhites | 0) (QUOTE systemId=maybeText QUOTE | 0) | 0)
    (SYSTEM maybeWhites QUOTE systemId=maybeText QUOTE | 0)
    maybeWhites
    ( copen=OBRACKET maybeWhites (#children=dtdChild maybeWhites)* maybeWhites cclose=CBRACKET | 0)
    maybeWhites
    ( tclose=GT | 0 [: reportProblem(Error, "Unclosed element"); :] )
->  dtdDoctype ;;


-----------------------------------------------------------------
-- Code segments copied to the implementation (.cpp) file.
-- If existent, kdevelop-pg's current syntax requires this block
-- to occur at the end of the file.
-----------------------------------------------------------------

[:

#include <QtCore/QDebug>
#include <KTextEditor/Range>
#include "sgmltokenizer.h"

namespace Xml
{

void Parser::tokenize(const QString& contents)
{
    m_contents = contents;
    Tokenizer *tokenizer;
    KMimeType::Ptr mime;
    if(!m_mime.isNull())
        mime = m_mime;
    else {
        
        mime = KMimeType::findByUrl(m_currentDocument.str());
    }
    if((!mime.isNull() && mime->is("application/xml-dtd"))
        || m_currentDocument.str().toLower().endsWith(".mod")
        || m_currentDocument.str().toLower().endsWith(".ent"))
        tokenizer = new DTDTokenizer(tokenStream, contents);
    else
        tokenizer = new SgmlTokenizer(tokenStream, contents);

    tokenizer->setCurrentDocument(m_currentDocument);
    tokenizer->setDtdHelper(DtdHelper::instanceForMime(mime));

    int kind = Parser::Token_EOF;

    do
    {
        kind = tokenizer->nextTokenKind();
        if ( !kind ) // when the tokenizer returns 0, the end of file is reached
        {
            kind = Parser::Token_EOF;
        }
        Parser::Token &t = tokenStream->next();
        t.begin = tokenizer->tokenBegin();
        t.end = tokenizer->tokenEnd();
        t.kind = kind;
        if ( m_debug ) qDebug() << kind << tokenText(t.begin,t.end) << t.begin << t.end;
    }
    while ( kind != Parser::Token_EOF  );

    m_dtdHelper = tokenizer->dtdHelper(); //If doctype found this will be non null.

    yylex(); // produce the look ahead token

    delete tokenizer;
}


QString Parser::tokenText(qint64 begin, qint64 end) const
{
    return m_contents.mid(begin, end - begin + 1);
}


QString Parser::tagName(const ElementTagAst *ast) const
{
    //DTD does not use prefixes, the whole prefixed name is its name
    if(!ast)
        return QString();
    if(ast->ns && ast->name)
        return tokenText(tokenStream->token(ast->ns->startToken).begin, tokenStream->token(ast->ns->endToken).end) +
                ":" +
                tokenText(tokenStream->token(ast->name->startToken).begin, tokenStream->token(ast->name->endToken).end);
    if(ast->name)
        return tokenText(tokenStream->token(ast->name->startToken).begin, tokenStream->token(ast->name->endToken).end);
    return QString();
}

QString Parser::tagName(const ElementCloseTagAst *ast) const
{
    //DTD does not use prefixes, the whole prefixed name is its name
    if(!ast)
        return QString();
    if(ast->ns && ast->name)
        return tokenText(tokenStream->token(ast->ns->startToken).begin, tokenStream->token(ast->ns->endToken).end) +
                ":" +
                tokenText(tokenStream->token(ast->name->startToken).begin, tokenStream->token(ast->name->endToken).end);
    if(ast->name)
        return tokenText(tokenStream->token(ast->name->startToken).begin, tokenStream->token(ast->name->endToken).end);
    return QString();
}

void Parser::reportProblem( Parser::ProblemType type, const QString& message )
{
    if (type == Error)
        qDebug() << "** ERROR:" << message;
    else if (type == Warning)
        qDebug() << "** WARNING:" << message;
    else if (type == Info)
        qDebug() << "** Info:" << message;

    qint64 sLine;
    qint64 sCol;
    qint64 index = tokenStream->index()-1;
    tokenStream->startPosition(index, &sLine, &sCol);
    qint64 eLine;
    qint64 eCol;
    tokenStream->endPosition(index, &eLine, &eCol);
    KDevelop::Problem *p = new KDevelop::Problem();
    p->setSource(KDevelop::ProblemData::Parser);
    switch ( type ) {
        case Error:
            p->setSeverity(KDevelop::ProblemData::Error);
            break;
        case Warning:
            p->setSeverity(KDevelop::ProblemData::Warning);
            break;
        case Info:
            p->setSeverity(KDevelop::ProblemData::Hint);
            break;
    }
    p->setDescription(message);
    p->setFinalLocation(KDevelop::DocumentRange(m_currentDocument, KTextEditor::Range(sLine, sCol, eLine, eCol+1)));
    m_problems << KDevelop::ProblemPointer(p);
}


// custom error recovery
void Parser::expectedToken(int /*expected*/, qint64 /*where*/, const QString& name)
{
    reportProblem( Parser::Error, QString("Expected token \"%1\"").arg(name));
}

void Parser::expectedSymbol(int /*expectedSymbol*/, const QString& name)
{
    qint64 line;
    qint64 col;
    qint64 index = tokenStream->index()-1;
    Token &token = tokenStream->token(index);
    kDebug() << "token starts at:" << token.begin;
    kDebug() << "index is:" << index;
    tokenStream->startPosition(index, &line, &col);
    QString tokenValue = tokenText(token.begin, token.end);
    qint64 eLine;
    qint64 eCol;
    tokenStream->endPosition(index, &eLine, &eCol);
    reportProblem( Parser::Error,
                   QString("Expected symbol \"%1\" (current token: \"%2\" [%3] at %4:%5 - %6:%7)")
                  .arg(name)
                  .arg(token.kind != 0 ? tokenValue : "EOF")
                  .arg(token.kind)
                  .arg(line)
                  .arg(col)
                  .arg(eLine)
                  .arg(eCol));
}

void Parser::setDebug( bool debug )
{
    m_debug = debug;
}

void Parser::setCurrentDocument(KDevelop::IndexedString url)
{
    m_currentDocument = url;
}

void Parser::setMime(KMimeType::Ptr mime) {
    m_mime = mime;
}

bool Parser::LAChoice(int &i) {
    while(++i < 9999 
        && LA(i).kind != Token_CPAREN
        && ({if(LA(i).kind == Token_OPAREN && LAChoice(i)); true;})
        && LA(i).kind != Token_EOF);
    return true;
}

} // end of namespace Xml

:]

-- kate: space-indent on; indent-width 4; tab-width 4; replace-tabs on; auto-insert-doxygen on
