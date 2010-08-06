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

#include "sgmltokenizertest.h"
#include "../sgmltokenizer.h"
#include "sgmlparser.h"

#include <QTest>
#include <dtdtokenizer.h>
#include <sgmltokentext.h>

QTEST_MAIN(Xml::TestSgmlTokenizer)

typedef QList<Xml::Parser::TokenType> TokenList;

Q_DECLARE_METATYPE(TokenList)

namespace Xml
{

TestSgmlTokenizer::TestSgmlTokenizer()
{
    qRegisterMetaType< TokenList >("TokenList");
}

void TestSgmlTokenizer::testDoctype_data()
{
    QTest::addColumn<QString>("token");
    QTest::addColumn<QString>("expectedName");
    QTest::addColumn<QString>("expectedPublicId");
    QTest::addColumn<QString>("expectedSystemId");

    QTest::newRow("doctype-html5") << "<!doctype html>" << "html" << "" << "";
    QTest::newRow("doctype-public") << "<!DOCTYPE html PUBLIC \"publicId\">" << "html" << "publicId" << "";
    QTest::newRow("doctype-public-system") << "<!DOCTYPE html PUBLIC \"publicId\" \"systemId\">" << "html" << "publicId" << "systemId";
    QTest::newRow("doctype-system") << "<!DOCTYPE html SYSTEM \"systemId\">" << "html" << "" << "systemId";
    //TODO: anything else to check?
    QTest::newRow("doctype-system-emtpy") << "<!DOCTYPE html SYSTEM>" << "html" << "" << "";
}

void TestSgmlTokenizer::testDoctype()
{
    QFETCH(QString, token);
    QFETCH(QString, expectedName);
    QFETCH(QString, expectedPublicId);
    QFETCH(QString, expectedSystemId);

    SgmlTokenizer tokenizer(NULL,"");
    QString name;
    QString publicId;
    QString systemId;
    tokenizer.doctype(token, name, publicId, systemId);
    QCOMPARE(name, expectedName);
    QCOMPARE(publicId, expectedPublicId);
    QCOMPARE(systemId, expectedSystemId);
}

void TestSgmlTokenizer::testElementName_data()
{
  QTest::addColumn<QString>("code");
  QTest::addColumn<QString>("name");

  QTest::newRow("open") << "<foo>" << "foo";
  QTest::newRow("close") << "</foo>" << "foo";
  QTest::newRow("open-ns") << "<ns:tagName>" << "ns:tagName";
  QTest::newRow("open-ns-empty") << "<ns:tagName/>" << "ns:tagName";
  QTest::newRow("open-ns-attrib") << "<ns:tagName attrib='value'/>" << "ns:tagName";
  QTest::newRow("close-ns") << "</ns:tagName >" << "ns:tagName";
  QTest::newRow("close-ns-whitespace") << "</\r\n\t ns:tagName >" << "ns:tagName";
}

void TestSgmlTokenizer::testElementName()
{
    QFETCH(QString, code);
    QFETCH(QString, name);
    SgmlTokenizer tokenizer(NULL,"");
    QString tagName = tokenizer.elementName(code);
    QCOMPARE(tagName, name);
}

void TestSgmlTokenizer::testReadUntill()
{
    
    SgmlTokenizer tokenizer(NULL,"");
    QString test = "-            < </script>>>.";
    const QChar *start = test.data();
    const QChar *ret = tokenizer.readUntill(test.data(), test.data()+test.length(), "</script>", SgmlTokenizer::IgnoreWhites, &start);
    QStringRef ref(&test, start - test.data(), ret - start);
    if(ref.toString() != "</script>")
        QFAIL(QString("EXPECTED '</script>' GOT '%1'").arg(ref.toString()).toAscii().constData());

    test = " --";
    start = test.data();
    ret = tokenizer.readUntill(test.data(), test.data()+test.length(), "--", SgmlTokenizer::IgnoreWhites, &start);
    ref = QStringRef(&test, start - test.data(), ret - start);
    if(ref.toString() != "--")
        QFAIL(QString("EXPECTED '--' GOT '%1'").arg(ref.toString()).toAscii().constData());
}

void TestSgmlTokenizer::testReadUntillAny()
{

    SgmlTokenizer tokenizer(NULL,"");
    QString test = "<![CDATA ] ] >";
    const QChar * ret = tokenizer.readUntillAny(test.data(), test.data()+test.length(), "]]>", SgmlTokenizer::IgnoreWhites);
    QStringRef ref(&test, 0, ret - test.data());
    if(QString("<![CDATA ") != ref.toString())
        QFAIL(QString("EXPECTED '%1' GOT '%2'").arg("<![CDATA ",ref.toString()).toAscii().constData());
    if(ret->unicode() != ']')
        QFAIL(QString("EXPECTED '%1' GOT '%2'").arg("]",*ret).toAscii().constData());
    tokenizer.readUntillAny(test.data(), test.data()+test.length(), "xxx", SgmlTokenizer::IgnoreWhites);

}

void TestSgmlTokenizer::testReadWhileAny()
{

    SgmlTokenizer tokenizer(NULL,"");
    QString test = "<![ CDATA ] ] >";
    const QChar *start = test.data();
    const QChar * ret = tokenizer.readWhileAny(test.data(), test.data()+test.length(), "<![", SgmlTokenizer::IgnoreWhites, &start);
    QStringRef ref(&test, 0, ret - test.data());
    if(QString("<![ ") != ref.toString())
        QFAIL(QString("EXPECTED '%1' GOT '%2'").arg("<![ ",ref.toString()).toAscii().constData());
    if(ret->unicode() != 'C')
        QFAIL(QString("EXPECTED '%1' GOT '%2'").arg("C",*ret).toAscii().constData());
    if(start->unicode() != '<')
        QFAIL(QString("EXPECTED '%1' GOT '%2'").arg("<",*start).toAscii().constData());
    tokenizer.readWhileAny(test.data(), test.data()+test.length(), "<![ CDATA ] ] >", SgmlTokenizer::IgnoreWhites, &start);

}
void TestSgmlTokenizer::testDTDTokenizer_data()
{
    QTest::addColumn<QString>("code");
    QTest::addColumn<TokenList>("tokens");

    {
    QList<Parser::TokenType> tokens;
    tokens.append(Parser::Token_DOCTYPE);
    tokens.append(Parser::Token_WHITE);
    tokens.append(Parser::Token_TEXT);
    tokens.append(Parser::Token_WHITE);
    tokens.append(Parser::Token_PUBLIC);
    tokens.append(Parser::Token_WHITE);
    tokens.append(Parser::Token_QUOTE);
    tokens.append(Parser::Token_TEXT);
    tokens.append(Parser::Token_QUOTE);
    tokens.append(Parser::Token_WHITE);
    tokens.append(Parser::Token_QUOTE);
    tokens.append(Parser::Token_TEXT);
    tokens.append(Parser::Token_QUOTE);
    tokens.append(Parser::Token_GT);
    QTest::newRow("") << "<!doctype root PUBLIC \"uri\" \"uri\">" << tokens;
    }

    {
    QList<Parser::TokenType> tokens;
    tokens.append(Parser::Token_ELEMENT);
    tokens.append(Parser::Token_WHITE);
    tokens.append(Parser::Token_TEXT);
    tokens.append(Parser::Token_WHITE);
    tokens.append(Parser::Token_OPAREN);
    tokens.append(Parser::Token_TEXT);
    tokens.append(Parser::Token_COMMA);
    tokens.append(Parser::Token_TEXT);
    tokens.append(Parser::Token_QUESTION);
    tokens.append(Parser::Token_CPAREN);
    tokens.append(Parser::Token_GT);
    QTest::newRow("") << "<!ELEMENT direction (left,top?)>" << tokens;
    }

    {
    QList<Parser::TokenType> tokens;
    tokens.append(Parser::Token_ELEMENT);
    tokens.append(Parser::Token_WHITE);
    tokens.append(Parser::Token_TEXT);
    tokens.append(Parser::Token_WHITE);
    tokens.append(Parser::Token_TEXT);
    tokens.append(Parser::Token_GT);
    QTest::newRow("") << "<!ELEMENT HR EMPTY>" << tokens;
    }

    {
    QList<Parser::TokenType> tokens;
    tokens.append(Parser::Token_ELEMENT);
    tokens.append(Parser::Token_WHITE);
    tokens.append(Parser::Token_TEXT);
    tokens.append(Parser::Token_WHITE);
    tokens.append(Parser::Token_OPAREN);
    tokens.append(Parser::Token_NUMBER);
    tokens.append(Parser::Token_TEXT);
    tokens.append(Parser::Token_VBAR);
    tokens.append(Parser::Token_TEXT);
    tokens.append(Parser::Token_CPAREN);
    tokens.append(Parser::Token_ASTERISK);
    tokens.append(Parser::Token_GT);
    QTest::newRow("") << "<!ELEMENT p (#PCDATA|I)*>" << tokens;
    }

    {
    QList<Parser::TokenType> tokens;
    tokens.append(Parser::Token_ELEMENT);
    tokens.append(Parser::Token_WHITE);
    tokens.append(Parser::Token_TEXT);
    tokens.append(Parser::Token_WHITE);
    tokens.append(Parser::Token_HYPHEN);
    tokens.append(Parser::Token_WHITE);
    tokens.append(Parser::Token_OPT);
    tokens.append(Parser::Token_WHITE);
    tokens.append(Parser::Token_TEXT);
    tokens.append(Parser::Token_GT);
    QTest::newRow("") << "<!ELEMENT P - O EMPTY>" << tokens;
    }

    {
    QList<Parser::TokenType> tokens;
    tokens.append(Parser::Token_ATTLIST); //<!ATTLIST
    tokens.append(Parser::Token_WHITE);
    tokens.append(Parser::Token_TEXT); //OPTIONS
    tokens.append(Parser::Token_WHITE);
    tokens.append(Parser::Token_TEXT); //ID
    tokens.append(Parser::Token_WHITE);
    tokens.append(Parser::Token_TEXT); //CDATA
    tokens.append(Parser::Token_WHITE);
    tokens.append(Parser::Token_NUMBER); //#
    tokens.append(Parser::Token_TEXT); //IMPLIED
    tokens.append(Parser::Token_WHITE);
    tokens.append(Parser::Token_TEXT); //NUMBER
    tokens.append(Parser::Token_WHITE);
    tokens.append(Parser::Token_TEXT); //CDATA
    tokens.append(Parser::Token_WHITE);
    tokens.append(Parser::Token_NUMBER); //#
    tokens.append(Parser::Token_TEXT); //IMPLIED
    tokens.append(Parser::Token_GT);
    QTest::newRow("") << "<!ATTLIST OPTIONS ID CDATA #IMPLIED NUMBER CDATA #IMPLIED>" << tokens;
    }

    {
    QList<Parser::TokenType> tokens;
    tokens.append(Parser::Token_ATTLIST);
    tokens.append(Parser::Token_WHITE);
    tokens.append(Parser::Token_TEXT);
    tokens.append(Parser::Token_WHITE);
    tokens.append(Parser::Token_TEXT);
    tokens.append(Parser::Token_WHITE);
    tokens.append(Parser::Token_OPAREN);
    tokens.append(Parser::Token_TEXT);
    tokens.append(Parser::Token_VBAR);
    tokens.append(Parser::Token_TEXT);
    tokens.append(Parser::Token_CPAREN);
    tokens.append(Parser::Token_WHITE);
    tokens.append(Parser::Token_QUOTE);
    tokens.append(Parser::Token_TEXT);
    tokens.append(Parser::Token_QUOTE);
    tokens.append(Parser::Token_GT);
    QTest::newRow("") << "<!ATTLIST WHERE CITY (JHB|PTA) 'JHB'--junk-->" << tokens;
    }

    {
    QList<Parser::TokenType> tokens;
    tokens.append(Parser::Token_ELEMENT);
    tokens.append(Parser::Token_WHITE);
    tokens.append(Parser::Token_OPAREN);
    tokens.append(Parser::Token_TEXT);
    tokens.append(Parser::Token_VBAR);
    tokens.append(Parser::Token_TEXT);
    tokens.append(Parser::Token_CPAREN);
    tokens.append(Parser::Token_WHITE);
    tokens.append(Parser::Token_HYPHEN);
    tokens.append(Parser::Token_WHITE);
    tokens.append(Parser::Token_OPT);
    tokens.append(Parser::Token_WHITE);
    tokens.append(Parser::Token_TEXT);
    tokens.append(Parser::Token_GT);
    QTest::newRow("") << "<!ELEMENT (SUB|SUP) - O EMPTY>" << tokens;
    }

    {
    QList<Parser::TokenType> tokens;
    tokens.append(Parser::Token_ATTLIST);
    tokens.append(Parser::Token_WHITE);
    tokens.append(Parser::Token_TEXT);
    tokens.append(Parser::Token_WHITE);
    tokens.append(Parser::Token_TEXT);
    tokens.append(Parser::Token_WHITE);
    tokens.append(Parser::Token_TEXT);
    tokens.append(Parser::Token_WHITE);
    tokens.append(Parser::Token_QUOTE);
    tokens.append(Parser::Token_TEXT);
    tokens.append(Parser::Token_QUOTE);
    tokens.append(Parser::Token_GT);
    QTest::newRow("") << "<!ATTLIST user role CDATA 'guest'>" << tokens;
    }

    {
    QList<Parser::TokenType> tokens;
    tokens.append(Parser::Token_ELEMENT);
    tokens.append(Parser::Token_WHITE);
    tokens.append(Parser::Token_TEXT);
    tokens.append(Parser::Token_WHITE);
    tokens.append(Parser::Token_OPAREN);
    tokens.append(Parser::Token_OPAREN);
    tokens.append(Parser::Token_TEXT);
    tokens.append(Parser::Token_VBAR);
    tokens.append(Parser::Token_TEXT);
    tokens.append(Parser::Token_CPAREN);
    tokens.append(Parser::Token_ASTERISK);
    tokens.append(Parser::Token_COMMA);
    tokens.append(Parser::Token_OPAREN);
    tokens.append(Parser::Token_TEXT);
    tokens.append(Parser::Token_COMMA);
    tokens.append(Parser::Token_TEXT);
    tokens.append(Parser::Token_ASTERISK);
    tokens.append(Parser::Token_CPAREN);
    tokens.append(Parser::Token_CPAREN);
    tokens.append(Parser::Token_GT);
    QTest::newRow("") << "<!ELEMENT name1 ((name2|name3)*,(name4,name5*))>" << tokens;
    }

    {
    QList<Parser::TokenType> tokens;
    tokens.append(Parser::Token_ATTLIST);
    tokens.append(Parser::Token_WHITE);
    tokens.append(Parser::Token_TEXT);
    tokens.append(Parser::Token_WHITE);
    tokens.append(Parser::Token_PERCENT);
    tokens.append(Parser::Token_TEXT);
    tokens.append(Parser::Token_SEMICOLON);
    tokens.append(Parser::Token_GT);
    QTest::newRow("") << "<!ATTLIST TITLE %i18n;>" << tokens;
    }
}

void TestSgmlTokenizer::testDTDTokenizer()
{
    QFETCH(QString, code);
    QFETCH(TokenList, tokens);

    DTDTokenizer tokenizer(NULL, code);
    int token;
    int encounteredTokens = 0;
    while((token = tokenizer.nextTokenKind()) != Parser::Token_EOF) {
        QCOMPARE(tokenText(token), tokenText(tokens[encounteredTokens]));
        encounteredTokens++;
    }
    QCOMPARE(encounteredTokens, tokens.size());
}

void TestSgmlTokenizer::testDTDEntity()
{
    QString text = "<!entity % --crap-- empty --crap-- 'EMPTY --crap--'><!entity % blah '<!element blah %empty; --crap-->'>%blah;";
    DTDTokenizer tokenizer(NULL, text);
    int token;
    int count = 0;
    while((token = tokenizer.nextTokenKind()) != Parser::Token_EOF) {
        QStringRef tokeStr(&text, tokenizer.tokenBegin(), tokenizer.tokenEnd() - tokenizer.tokenBegin() + 1);
        kDebug() << count << tokeStr.toString();
        count++;
    }
    if(count != 30)
        QFAIL(QString("In [%1] Incorrect number of tokens").arg(text).toAscii().constData());
}

void TestSgmlTokenizer::testSgmlTokenizer_data()
{
    QTest::addColumn<QString>("code");
    QTest::addColumn<TokenList>("tokens");

    {
    QList<Parser::TokenType> tokens;
    tokens.append(Parser::Token_LT);
    tokens.append(Parser::Token_TEXT);
    tokens.append(Parser::Token_COLON);
    tokens.append(Parser::Token_TEXT);
    tokens.append(Parser::Token_WHITE);
    tokens.append(Parser::Token_TEXT);
    tokens.append(Parser::Token_EQUAL);
    tokens.append(Parser::Token_TEXT);
    tokens.append(Parser::Token_CLOSE);
    QTest::newRow("nstag-attrib-empty") << "<ns:name parm=''/>" << tokens;
    }

    {
    QList<Parser::TokenType> tokens;
    tokens.append(Parser::Token_COMMENT);
    QTest::newRow("comment") << "<!--  -->" << tokens;
    }

    {
    QList<Parser::TokenType> tokens;
    tokens.append(Parser::Token_CDATA);
    QTest::newRow("cdata") << "<![CDATA[ sdsd ]]>" << tokens;
    }

    {
    QList<Parser::TokenType> tokens;
    tokens.append(Parser::Token_PHP);
    QTest::newRow("php") << "<?php ?>" << tokens;
    }

    {
    QList<Parser::TokenType> tokens;
    tokens.append(Parser::Token_PROC);
    tokens.append(Parser::Token_CDATA);
    QTest::newRow("proc-cdata") << "<?xml?> <![CDATA[]]>" << tokens;
    }

    {
    QList<Parser::TokenType> tokens;
    tokens.append(Parser::Token_DOCTYPE);
    tokens.append(Parser::Token_WHITE);
    tokens.append(Parser::Token_TEXT);
    tokens.append(Parser::Token_WHITE);
    tokens.append(Parser::Token_GT);
    QTest::newRow("doctype") << "<!DOCTYPE root >" << tokens;
    }

    {
    QList<Parser::TokenType> tokens;
    tokens.append(Parser::Token_DOCTYPE);
    tokens.append(Parser::Token_WHITE);
    tokens.append(Parser::Token_TEXT);
    tokens.append(Parser::Token_WHITE);
    tokens.append(Parser::Token_PUBLIC);
    tokens.append(Parser::Token_WHITE);
    tokens.append(Parser::Token_QUOTE);
    tokens.append(Parser::Token_TEXT);
    tokens.append(Parser::Token_QUOTE);
    tokens.append(Parser::Token_WHITE);
    tokens.append(Parser::Token_QUOTE);
    tokens.append(Parser::Token_TEXT);
    tokens.append(Parser::Token_QUOTE);
    tokens.append(Parser::Token_GT);
    QTest::newRow("doctype-public") << "<!doctype root PUBLIC \"uri\" \"uri\">" << tokens;
    }

    {
    QList<Parser::TokenType> tokens;
    tokens.append(Parser::Token_LT);
    tokens.append(Parser::Token_TEXT);
    tokens.append(Parser::Token_WHITE);
    tokens.append(Parser::Token_TEXT);
    tokens.append(Parser::Token_EQUAL);
    tokens.append(Parser::Token_TEXT);
    tokens.append(Parser::Token_WHITE);
    tokens.append(Parser::Token_TEXT);
    tokens.append(Parser::Token_COLON);
    tokens.append(Parser::Token_TEXT);
    tokens.append(Parser::Token_EQUAL);
    tokens.append(Parser::Token_TEXT);
    tokens.append(Parser::Token_WHITE);
    tokens.append(Parser::Token_TEXT);
    tokens.append(Parser::Token_COLON);
    tokens.append(Parser::Token_TEXT);
    tokens.append(Parser::Token_EQUAL);
    tokens.append(Parser::Token_TEXT);
    tokens.append(Parser::Token_GT);
    QTest::newRow("kcfg-attribs") << "<kcfg xmlns=\"http://www.kde.org/standards/kcfg/1.0\" "
                                     "xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" "
                                     "xsi:schemaLocation=\"http://www.kde.org/standards/kcfg/1.0 "
                                     "http://www.kde.org/standards/kcfg/1.0/kcfg.xsd\">"
                                  << tokens;
    }
}

void TestSgmlTokenizer::testSgmlTokenizer()
{
    QFETCH(QString, code);
    QFETCH(TokenList, tokens);

    SgmlTokenizer tokenizer(NULL, code);
    int token;
    int encounteredTokens = 0;
    while((token = tokenizer.nextTokenKind()) != Parser::Token_EOF) {
        QCOMPARE(tokenText(token), tokenText(tokens[encounteredTokens]));
        encounteredTokens++;
    }
    QEXPECT_FAIL("php", "No tokens are found for PHP test case", Continue);
    QCOMPARE(encounteredTokens, tokens.size());

}

void TestSgmlTokenizer::testTokenizerForFun()
{
    
    {
        QString text = "<!DOCTYPE html>\n\
        <head>\n\
            <meta>\n\
        </head>";
        SgmlTokenizer tokenizer(NULL, text);
        int token;
        while ((token = tokenizer.nextTokenKind()) != Parser::Token_EOF) {
            kDebug() << text.mid(tokenizer.tokenBegin(), tokenizer.tokenEnd() - tokenizer.tokenBegin() + 1);
        }
    }
    
    {
        QString text = "<!DOCTYPE html>\n\
<html>\n\
    <head>\n\
        <script type='java'> \n\
        vfdc <  \n\
        </script><blah>\n\
        <table>\n\
            <tr>\n\
                <td/>\n\
            </tr>\n\
        </table>\n\
    </head>\n\
</html>\n\
";
        SgmlTokenizer tokenizer(NULL, text);
        int token;
        while ((token = tokenizer.nextTokenKind()) != Parser::Token_EOF) {
            kDebug() << text.mid(tokenizer.tokenBegin(), tokenizer.tokenEnd() - tokenizer.tokenBegin() + 1);
        }
    }
}

}

#include "sgmltokenizertest.moc"
