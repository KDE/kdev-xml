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
#include <schemabuilder.h>


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
    //Syncronise parsing
    KDevelop::UrlParseLock urlLock(document());

    KUrl documentUrl = document().toUrl();
    KMimeType::Ptr documentMime = KMimeType::findByUrl(documentUrl);
    
    if ( !(minimumFeatures() & TopDUContext::ForceUpdate || minimumFeatures() & Resheduled) ) {
        bool needsUpdate = true;
        DUChainReadLocker lock(DUChain::lock());
        static const IndexedString langString("Xml");
        foreach(const ParsingEnvironmentFilePointer &file, DUChain::self()->allEnvironmentFiles(document())) {
            if (file->language() != langString) {
                continue;
            }
            if (!file->needsUpdate()) {
                kDebug() << "Already up to date" << document().str();
                setDuChain(file->topContext());
                needsUpdate = false;
                break;
            }
        }
        
        //Update if we do not have the scema
        if(documentMime->is("application/xsd")) {
            SchemaMutexLocker lock;
            if(!SchemaController::self()->schemaForDocument(document()))
                needsUpdate = true;
        }
        
        if (!needsUpdate) {
            debug() << "Already up to date" << document().str();
            return;
        }
    }

    debug() << "Parsing:" << document().str();

    KDevelop::ProblemPointer p = readContents();
    if (p) {
        //TODO: associate problem with topducontext
        return abortJob();;
    }
    
    //Dont include yourself
    m_includes.insert(document().str(), QString::null);

    //Tokenize and parse
    ParseSession session;
    session.setMime(documentMime);
    //TODO: support different charsets
    session.setContents(QString::fromUtf8(contents().contents));
    session.setCurrentDocument(document());


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
    editor.setMime(documentMime);
    editor.setCurrentUrl(document());

    //Build includes
    IncludeBuilder includeBuilder(&editor);
    includeBuilder.build(document(), start);
    problems.append(includeBuilder.problems());
    foreach(const IncludeBuilder::IncludeIdentifier &inc, includeBuilder.includes().values()) {
        parseInclude(inc, top);
    }

    //Build the declarations
    DeclarationBuilder builder(&editor);
    top = builder.build(document(), start, top);
    problems.append(builder.problems());

    //Set the chain the schema builder needs it
    setDuChain(top);

    //Build the XSD schema
    if (documentMime->is("application/xsd")) {
        SchemaBuilder builder(&editor);
        builder.build(document(), start);
        problems.append(builder.problems());
    }

    //Report all problems
    foreach(const KDevelop::ProblemPointer &p, problems) {
        KDevelop::DUChainWriteLocker lock(KDevelop::DUChain::lock());
        top->addProblem(p);
    }

    //Finalise
    {
        KDevelop::DUChainWriteLocker lock(KDevelop::DUChain::lock());

        top->setFeatures(minimumFeatures());
        KDevelop::ParsingEnvironmentFilePointer file = top->parsingEnvironmentFile();
        QFileInfo fileInfo(document().str());
        QDateTime lastModified = fileInfo.lastModified();
        file->setModificationRevision(KDevelop::ModificationRevision(lastModified));
        KDevelop::DUChain::self()->updateContextEnvironment( top->topContext(), file.data() );
    }

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
    if (d->kind() == KDevelop::Declaration::Namespace)
        debug() << "Namespace:" << d->qualifiedIdentifier().toString();
    if (d->kind() == KDevelop::Declaration::Type)
        debug() << "Type     :" << d->qualifiedIdentifier().toString();
    if (d->kind() == KDevelop::Declaration::Instance)
        debug() << "Instance :" << d->qualifiedIdentifier().toString();
    if (d->kind() == KDevelop::Declaration::Import)
        debug() << "Import   :" << d->qualifiedIdentifier().toString();
    if (d->kind() == KDevelop::Declaration::Alias)
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

    if (m_includes.contains(url.pathOrUrl()))
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
