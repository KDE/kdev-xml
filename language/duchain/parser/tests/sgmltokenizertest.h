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

#ifndef SGMLTOKENIZERTEST_H
#define SGMLTOKENIZERTEST_H

#include <QObject>

#include "tests/duchaintestbase.h"

namespace Xml
{

class TestSgmlTokenizer : public DUChainTestBase
{
    Q_OBJECT

public:
    TestSgmlTokenizer();

private Q_SLOTS:
    void testDoctype_data();
    void testDoctype();
    void testElementName_data();
    void testElementName();
    void testReadUntill();
    void testReadUntillAny();
    void testReadWhileAny();
    void testSgmlTokenizer_data();
    void testSgmlTokenizer();
    void testDTDTokenizer_data();
    void testDTDTokenizer();
    void testDTDEntity();
    void testTokenizerForFun();
};
}

#endif //SGMLTOKENIZERTEST_H
