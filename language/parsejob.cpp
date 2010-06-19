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

#include "parsejob.h"
#include "languagesupport.h"
#include "parsesession.h"
#include "sgmlparser.h"
#include "declarationbuilder.h"
#include "includebuilder.h"
#include "editorintegrator.h"
#include "language_debug.h"

#include <xmlcatalog/icatalogmanager.h>
#include <xmlcatalog/catalogmanager.h>
#include <xmlcatalog/cataloghelper.h>
#include <xmlcatalog/idocumentcachemanager.h>

#include <QFile>
#include <QReadWriteLock>
#include <QtCore/QReadLocker>
#include <QtCore/QThread>

#include <KMimeType>

#include <interfaces/ilanguage.h>
#include <language/duchain/duchainlock.h>
#include <language/duchain/duchain.h>
#include <language/duchain/topducontext.h>
#include <language/duchain/parsingenvironment.h>
#include <language/duchain/declaration.h>
#include <language/backgroundparser/urlparselock.h>
#include <language/duchain/duchainutils.h>


namespace Xml
{

ParseJob::ParseJob(const KUrl& url) : KDevelop::ParseJob(url)
{
    debug();
}

ParseJob::~ParseJob()
{
    debug();
}

void ParseJob::run()
{
    KDevelop::UrlParseLock urlLock(document());

    if ( !(minimumFeatures() & KDevelop::TopDUContext::ForceUpdate) ) {
        KDevelop::DUChainReadLocker lock(KDevelop::DUChain::lock());
        bool needsUpdate = true;
        foreach(const KDevelop::ParsingEnvironmentFilePointer &file, KDevelop::DUChain::self()->allEnvironmentFiles(document())) {
            if (file->needsUpdate()) {
                needsUpdate = true;
                break;
            } else {
                needsUpdate = false;
            }
        }
        if (!needsUpdate) {
            debug() << "Already up to date" << document().str();
            cleanupSmartRevision();
            return;
        }
    }

    debug() << "Parsing:" << document().str();
    //Dont include yourself
    m_includes.insert(document().str(), QString::null);

    //Tokenize
    ParseSession session;
    if (!document().str().isEmpty()) {
        session.setCurrentDocument(document().str());
    }
    bool readFromDisk = !contentsAvailableFromEditor();
    if (readFromDisk) {
        session.readFile(document().str());
    } else {
        session.setContents(contentsFromEditor());
    }
    
    StartAst *start=0;
    if (!session.parse(&start)) {
        debug() << "Failed to parse:" << document().str();
    }
 
    //Build
    debug() << "Building:" << document().str();
    KDevelop::ReferencedTopDUContext top;
    {
        KDevelop::DUChainReadLocker lock(KDevelop::DUChain::lock());
        top = KDevelop::DUChain::self()->chainForDocument(document());
    }
 
    if (top) {
        debug() << "Re-compiling:" << document().str();
        KDevelop::DUChainWriteLocker lock(KDevelop::DUChain::lock());
        top->clearImportedParentContexts();
        if (top->parsingEnvironmentFile())
            top->parsingEnvironmentFile()->clearModificationRevisions();
        top->clearProblems();
    } else {
        debug() << "Compiling:" << document().str();
    }
    
    QList<KDevelop::ProblemPointer> problems;
    problems.append(session.problems());

    EditorIntegrator editor(&session);
    IncludeBuilder includeBuilder(&editor);
    includeBuilder.build(document(), start);
    problems.append(includeBuilder.problems());
    foreach(const IncludeBuilder::IncludeIdentifier &inc, includeBuilder.includes().values()) {
        parseInclude(inc, top);
    }
    
    //if(KMimeType::findByUrl(document().toUrl())->is("application/xsd")) {
    //    XsdDeclarationBuilder builder(&editor);
    //    top = builder.build(document(), start, top);
    //    problems.append(builder.problems());
    //} else {
        DeclarationBuilder builder(&editor);
        top = builder.build(document(), start, top);
        problems.append(builder.problems());
    //}
    setDuChain(top);

    foreach(const KDevelop::ProblemPointer &p, problems) {
        KDevelop::DUChainWriteLocker lock(KDevelop::DUChain::lock());
        top->addProblem(p);
    }

    {
        KDevelop::DUChainWriteLocker lock(KDevelop::DUChain::lock());

        top->setFeatures(minimumFeatures());
        KDevelop::ParsingEnvironmentFilePointer file = top->parsingEnvironmentFile();
        QFileInfo fileInfo(document().str());
        QDateTime lastModified = fileInfo.lastModified();
        if (readFromDisk) {
            file->setModificationRevision(KDevelop::ModificationRevision(lastModified));
        } else {
            file->setModificationRevision(KDevelop::ModificationRevision(lastModified, revisionToken()));
        }
        KDevelop::DUChain::self()->updateContextEnvironment( top->topContext(), file.data() );
    }

    cleanupSmartRevision();
    
    /*
    {
        KDevelop::DUChainReadLocker loc;
        foreach(KDevelop::Declaration *d1 , top->localDeclarations()) {
            visit(d1);
        }
        foreach(KDevelop::DUContext::Import c1, top->importedParentContexts()) {
            foreach(KDevelop::Declaration *d1 , c1.indexedContext().context()->localDeclarations()) {
                visit(d1);
            }
        }
    }
    */
}


void ParseJob::visit(KDevelop::Declaration* d)
{
    if (!d) return;
    if(d->kind() == KDevelop::Declaration::Namespace)
        debug() << "Namespace:" << d->qualifiedIdentifier().toString();
    if(d->kind() == KDevelop::Declaration::Type)
        debug() << "Type     :" << d->qualifiedIdentifier().toString();
    if(d->kind() == KDevelop::Declaration::Instance)
        debug() << "Instance :" << d->qualifiedIdentifier().toString();
    if(d->kind() == KDevelop::Declaration::Import)
        debug() << "Import   :" << d->qualifiedIdentifier().toString();
    if(d->kind() == KDevelop::Declaration::Alias)
        debug() << "Alias    :" << d->qualifiedIdentifier().toString();
    if (d->internalContext()) {
        foreach(KDevelop::Declaration *d1 , d->internalContext()->localDeclarations()) {
            visit(d1);
        }
        foreach(KDevelop::DUContext::Import c1, d->internalContext()->importedParentContexts()) {
            foreach(KDevelop::Declaration *d1 , c1.indexedContext().context()->localDeclarations()) {
                visit(d1);
            }
        }
    }
}

void ParseJob::parseInclude(const IncludeBuilder::IncludeIdentifier &include, KDevelop::ReferencedTopDUContext top)
{
    Q_UNUSED(top);
    
    KUrl url = CatalogHelper::resolve(include.publicId.str(),
                                      include.systemId.str(),
                                      include.uri.str(),
                                      include.doctype.str(),
                                      KMimeType::Ptr(),
                                      document().toUrl());
    if (!url.isValid())
        return;

    if(m_includes.contains(url.pathOrUrl()))
        return;
    
    m_includes.insert(url.pathOrUrl(), QString::null);
    
    ParseJob job(url);
    job.m_includes = m_includes;
    job.run();
}


LanguageSupport* ParseJob::sgml() const
{
    return LanguageSupport::self();
}

}

#include "parsejob.moc"
// kate: space-indent on; indent-width 4; tab-width 4; replace-tabs on; auto-insert-doxygen on
