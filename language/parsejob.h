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

#ifndef ML_PARSEJOB_H
#define ML_PARSEJOB_H

#include <language/backgroundparser/parsejob.h>

#include <QStringList>

#include <ksharedptr.h>
#include <ktexteditor/range.h>
#include <language/interfaces/iproblem.h>

#include "duchain/includebuilder.h"

namespace Xml
{
class AstNode;
class LanguageSupport;
class EditorIntegrator;

class ParseJob : public KDevelop::ParseJob
{
    enum {
        Resheduled = KDevelop::TopDUContext::LastFeature
    };
    
    Q_OBJECT

public:
    explicit ParseJob(const KUrl &url);
    
    virtual ~ParseJob();

    LanguageSupport* sgml() const;

protected:
    virtual void run();
    void visit(KDevelop::Declaration *);
    void parseInclude(const IncludeBuilder::IncludeIdentifier &include, KDevelop::ReferencedTopDUContext top);
    
    //Keep a list of already parsed files
    //Because the include builder runs before the contexts been created we need to
    //take care of recursion
    QHash<QString, QString> m_includes;
};

}

#endif
// kate: space-indent on; indent-width 4; tab-width 4; replace-tabs on; auto-insert-doxygen on
