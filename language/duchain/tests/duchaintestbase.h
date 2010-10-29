/* This file is part of KDevelop
    Copyright 2006 Hamish Rodda <rodda@kde.org>
    Copyright 2008,2010 Niko Sams <niko.sams@gmail.com>

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

#ifndef DUCHAINTESTBASE_H
#define DUCHAINTESTBASE_H

#include <QObject>
#include <QByteArray>
#include <QTest>
#include <language/duchain/dumpchain.h>
#include <language/duchain/duchain.h>
#include <language/duchain/duchainlock.h>

namespace KDevelop
{
class TopDUContext;
}

namespace Xml
{
/**
 * Manage pointer to TopDUContexts and release them properly, even if a test fails
 */
struct DUChainReleaser {
    DUChainReleaser(KDevelop::TopDUContext* top) : m_top(top) {}
    ~DUChainReleaser() {
        KDevelop::DUChainWriteLocker lock(KDevelop::DUChain::lock());
        KDevelop::DUChain::self()->removeDocumentChain(m_top);
    }
    KDevelop::TopDUContext* m_top;
};

class DUChainTestBase : public QObject
{
    Q_OBJECT

public:
    DUChainTestBase();

public:
    enum DumpArea {
        DumpNone = 0,
        DumpAST = 1,
        DumpDUChain = 2,
        DumpAll = 3
    };
    Q_DECLARE_FLAGS(DumpAreas, DumpArea)

public slots:
    void initTestCase();

protected:
    KDevelop::TopDUContext* parse(const QByteArray& unit, DumpAreas dump = static_cast<DumpAreas>(DumpAST | DumpDUChain), QString fileName = QString());

    //KDevelop::DumpChain dumper;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(DUChainTestBase::DumpAreas)
}

#endif
