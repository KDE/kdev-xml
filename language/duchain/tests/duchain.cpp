/* This file is part of KDevelop
    Copyright 2010 Niko Sams <niko.sams@gmail.com>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "duchain.h"

#include <QtTest/QtTest>

#include <language/duchain/parsingenvironment.h>
#include <language/duchain/duchain.h>
#include <language/duchain/duchainlock.h>
#include <language/duchain/topducontext.h>
#include <language/duchain/declaration.h>

#include "elementdeclaration.h"
#include "importdeclaration.h"

QTEST_MAIN(Xml::TestDUChain)

namespace Xml
{

TestDUChain::TestDUChain()
{
}

void TestDUChain::testXmlContext()
{
    //                 01234567890123456
    //Declare range    {    }
    //Context range          {   }
    QByteArray method("<root>blah</root>");

    KDevelop::TopDUContext* top = parse(method, DumpAll);
    DUChainReleaser releaseTop(top);
    KDevelop::DUChainWriteLocker lock(KDevelop::DUChain::lock());

    QCOMPARE(top->childContexts().count(), 1);
    QEXPECT_FAIL("", "The range includes the closing tag, i.e. extends until (0, 17)", Continue);
    QVERIFY(top->childContexts().first()->range() == KDevelop::SimpleRange(0, 6, 0, 10));
    KDevelop::Declaration *dec = top->findDeclarationAt(KDevelop::SimpleCursor(0, 1));
    QCOMPARE(dec->qualifiedIdentifier().toString(), QString("root")); //TODO: this should not include the space
    QVERIFY(dec->range() == KDevelop::SimpleRange(0, 1, 0, 5));
}

void TestDUChain::testSgmlContext()
{
    //                 0         1         2         3         4         5
    //                 012345678901234567890123456789012345678901234567890123456789
    //Declare range    {    }
    //Context range          {   }
    QByteArray method("<!doctype html><html><body><br><p>Text</body></html>");
    
    KDevelop::TopDUContext* top = parse(method, DumpAll);
    DUChainReleaser releaseTop(top);
    KDevelop::DUChainWriteLocker lock(KDevelop::DUChain::lock());
    
    QCOMPARE(top->childContexts().count(), 1);
    QVERIFY(top->childContexts().first()->range() == KDevelop::SimpleRange(0, 21, 0, 45));
    KDevelop::Declaration *dec = top->findDeclarationAt(KDevelop::SimpleCursor(0, 0));
    QCOMPARE(dec->qualifiedIdentifier().toString(), QString("DOCTYPE")); //TODO: this should not include the space
    QVERIFY(dec->range() == KDevelop::SimpleRange(0, 0, 0, 15));
}

void TestDUChain::testEmptyDTDAttlist()
{
  ///TODO: make this work properly
  QByteArray method("<!ELEMENT foo EMPTY>\n"
                    "<!ATTLIST foo\n>");
  KDevelop::TopDUContext* top = parse(method, DumpAll);
  DUChainReleaser releaseTop(top);
}

void TestDUChain::testUnclosedChild()
{
  QByteArray method("<parent><child></parent>");
  KDevelop::TopDUContext* top = parse(method, DumpAll);
  DUChainReleaser releaseTop(top);

  KDevelop::DUChainReadLocker lock;
  QEXPECT_FAIL("", "the close tag declaration is added to <child> since that one is not closed...", Abort);
  QCOMPARE(top->childContexts().first()->localDeclarations().count(), 2);
  ElementDeclaration* close = dynamic_cast<ElementDeclaration*>(top->childContexts().first()->localDeclarations().last());
  QVERIFY(close);
  QCOMPARE(close->elementType(), ElementDeclarationData::CloseTag);
  QCOMPARE(close->name().str(), QString("parent"));
}

void TestDUChain::testDoctype()
{
    QByteArray dtd("<!ENTITY foo \"bar\" >");
    KDevelop::TopDUContext* dtdCtx = parse(dtd, DumpAll, "/internal/test.dtd");
    QVERIFY(dtdCtx);
    DUChainReleaser releaseDtdCtx(dtdCtx);

    QByteArray xml("<!DOCTYPE test SYSTEM \"test.dtd\">");
    KDevelop::TopDUContext* xmlCtx = parse(xml, DumpAll);
    DUChainReleaser releaseXmlCtx(xmlCtx);

    KDevelop::DUChainReadLocker lock;
    QCOMPARE(xmlCtx->findDeclarations(QualifiedIdentifier("/internal/test.dtd")).count(), 1);
    QCOMPARE(xmlCtx->localDeclarations().count(), 1);
    QVERIFY(dynamic_cast<ImportDeclaration*>(xmlCtx->localDeclarations().first()));
    QCOMPARE(xmlCtx->localDeclarations().first()->kind(), Declaration::Import);
    QCOMPARE(xmlCtx->localDeclarations().first()->url().str(), QString("/internal/test.dtd"));
}

}

#include "duchain.moc"
