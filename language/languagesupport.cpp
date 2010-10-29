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

#include "languagesupport.h"

#include "completion/sgmlcodecompletionmodel.h"
#include "parsejob.h"
#include "language_debug.h"

#include <KDE/KPluginFactory>
#include <KDE/KPluginLoader>
#include <KDE/KAboutData>
#include <KTextEditor/Document>

#include <language/codecompletion/codecompletion.h>
#include <language/duchain/duchainutils.h>
#include <language/duchain/use.h>
#include <language/duchain/duchain.h>
#include <language/duchain/duchainlock.h>
#include <language/duchain/declaration.h>
#include <interfaces/idocument.h>
#include <interfaces/icore.h>
#include <interfaces/idocumentcontroller.h>
#include "duchain/navigation/navigationwidget.h"


K_PLUGIN_FACTORY(KDevXmlSupportFactory, registerPlugin<Xml::LanguageSupport>();)
K_EXPORT_PLUGIN(KDevXmlSupportFactory(KAboutData("kdevsgmlsupport","kdevsgml", ki18n("Markup Language Support"), "0.1",
                                      ki18n("Support for markup languages"), KAboutData::License_GPL)
                                      .addAuthor(ki18n("Ruan Strydom"), ki18n("Author"), "ruan@jcell.co.za", "")))

namespace Xml
{
LanguageSupport* LanguageSupport::m_self = 0;

LanguageSupport::LanguageSupport(QObject* parent, const QVariantList& args) :
        KDevelop::IPlugin(KDevXmlSupportFactory::componentData(), parent),
        KDevelop::ILanguageSupport()
{
    Q_UNUSED(args);
    KDEV_USE_EXTENSION_INTERFACE(KDevelop::ILanguageSupport)

    m_self = this;
    SgmlCodeCompletionModel* ccModel = new SgmlCodeCompletionModel(this);
    new KDevelop::CodeCompletion(this, ccModel, name());
}

QString LanguageSupport::name() const
{
    return "Sgml";
}

LanguageSupport* LanguageSupport::self()
{
    return m_self;
}

QWidget* LanguageSupport::specialLanguageObjectNavigationWidget(const KUrl& url, const KDevelop::CursorInRevision& position)
{
    KDevelop::TopDUContextPointer top;

    KDevelop::DUChainReadLocker lock(KDevelop::DUChain::lock());
    top = KDevelop::DUChain::self()->chainForDocument(url);
    if (!top) {
        debug() << "No top context";
        return 0;
    }

    KDevelop::DUContext *context = top->findContext(position);
    if (!context) {
        context = top.data();
    }

    if (!context) {
        debug() << "No context";
        return 0;
    }

    KDevelop::Declaration *decl = context->findDeclarationAt(position);
    if (!decl) {
        //In Xsd and xml some declarations may appear in the wrong context ranges
        if (context->parentContext())
            decl = context->parentContext()->findDeclarationAt(position);
        if (!decl) {
            debug() << "No declaration for cursor";
            return 0;
        }
    }

    return new NavigationWidget(KDevelop::DeclarationPointer(decl), top);
}

KDevelop::SimpleRange LanguageSupport::specialLanguageObjectRange(const KUrl& url, const KDevelop::CursorInRevision& position)
{
    /*
        KDevelop::TopDUContextPointer top;
        {
            KDevelop::DUChainReadLocker lock(KDevelop::DUChain::lock());
            top = KDevelop::DUChain::self()->chainForDocument(url);
            if (!top) {
                kDebug() << "No top context";
                return KDevelop::SimpleRange::invalid();
            }
            KDevelop::Declaration * decl = KDevelop::DUChainUtils::itemUnderCursor(url, position);
            if (!decl) {
                kDebug() << "No declaration for cursor";
                return KDevelop::SimpleRange::invalid();
            }
            return decl->range();
        }
    */
    return KDevelop::ILanguageSupport::specialLanguageObjectRange(url, position.castToSimpleCursor());
}

KDevelop::ParseJob* LanguageSupport::createParseJob(const KUrl& url)
{
    return new ParseJob(url);
}

}

#include "languagesupport.moc"
