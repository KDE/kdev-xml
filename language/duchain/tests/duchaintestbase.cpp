/* This file is part of KDevelop
    Copyright 2006 Hamish Rodda <rodda@kde.org>
    Copyright 2008 Niko Sams <niko.sams@gmail.com>

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

#include "duchaintestbase.h"

#include <QtTest/QtTest>

#include <language/duchain/parsingenvironment.h>
#include <language/duchain/duchainlock.h>
#include <language/duchain/topducontext.h>
#include <language/duchain/indexedstring.h>
#include <kstandarddirs.h>
#include <kcomponentdata.h>

#include "parsesession.h"
#include "sgmldebugvisitor.h"
#include "declarationbuilder.h"
#include "contextbuilder.h"
#include "editorintegrator.h"

#include <tests/autotestshell.h>
#include <tests/testcore.h>
#include <language/codegen/coderepresentation.h>

using namespace Xml;


DUChainTestBase::DUChainTestBase()
{
    KComponentData kd("kdevcsssupport");
}

void DUChainTestBase::initTestCase()
{
    KDevelop::AutoTestShell::init();
    KDevelop::TestCore* core = new KDevelop::TestCore();
    core->initialize(KDevelop::Core::NoUi);

    KDevelop::DUChain::self()->disablePersistentStorage();
    KDevelop::CodeRepresentation::setDiskChangesForbidden(true);
}

KDevelop::TopDUContext* DUChainTestBase::parse(const QByteArray& unit, DumpAreas dump, QString url)
{
    if (dump)
        kDebug() << "==== Beginning new test case...:" << endl << unit;

    ParseSession* session = new ParseSession();
    session->setContents(unit);
    StartAst* ast = 0;
    if (!session->parse(&ast)) {
        kDebug() << "Parse failed";
        return 0;
    }

    if (dump & DumpAST) {
        kDebug() << "===== AST:";
        DebugVisitor debugVisitor(session->tokenStream(), session->contents());
        debugVisitor.visitNode(ast);
    }

    static int testNumber = 0;
    if (url.isEmpty()) url = QString("file:///internal/%1").arg(testNumber++);

    EditorIntegrator editor(session);
    DeclarationBuilder builder(&editor);
    KDevelop::TopDUContext* top = builder.build(KDevelop::IndexedString(url), ast);

    if (dump & DumpDUChain) {
        kDebug() << "===== DUChain:";

        KDevelop::DUChainWriteLocker lock(KDevelop::DUChain::lock());
        //dumper.dump(top);
    }

    if (dump)
        kDebug() << "===== Finished test case.";

    delete session;

    return top;
}


#include "duchaintestbase.moc"

