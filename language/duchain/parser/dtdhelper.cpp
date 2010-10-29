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

#include "dtdhelper.h"
#include <elementdeclaration.h>

#include <QtCore/QHash>
#include <QtCore/QString>
#include <QtCore/QThread>

#include <xmlcatalog/cataloghelper.h>
#include <xmlcatalog/publicid.h>

#include <threadweaver/ThreadWeaver.h>
#include <threadweaver/WeaverInterface.h>

#include <interfaces/icore.h>
#include <interfaces/iruncontroller.h>
#include <interfaces/ilanguagecontroller.h>
#include <interfaces/ilanguage.h>
#include <language/duchain/topducontext.h>
#include <language/duchain/duchain.h>
#include <language/duchain/duchainlock.h>
#include <language/duchain/declaration.h>
#include <language/interfaces/ilanguagesupport.h>
#include <language/backgroundparser/parsejob.h>
#include <language/backgroundparser/backgroundparser.h>


using namespace Xml;
using namespace KDevelop;

DtdHelper::DtdHelper() : AbstractDtdHelper() {}

DtdHelper::~DtdHelper() {}

DtdHelper::DtdHelper(DtdHelperData* helper): AbstractDtdHelper(), d(helper) {}


class DUChainDTDHelper : public DtdHelperData {
public:
    DUChainDTDHelper() : DtdHelperData() {};
    DUChainDTDHelper(IndexedTopDUContext top) : DtdHelperData() {
        topContext = top;
        DUChainReadLocker lock;
        kDebug() << top.url().str();
    }

    virtual ~DUChainDTDHelper() {
        kDebug();
    }

    virtual bool isNull() const {
        return !topContext.isValid();
    }

    virtual bool cdataElement(const QString& elementName) const {
        if (isNull()) return false;
        DUChainReadLocker lock;
        QList<Declaration *> list = findDeclarations(elementName);
        foreach(Declaration * dec, list) {
            if (!dec) continue;
            ElementDeclaration *elementDec = dynamic_cast<ElementDeclaration *>(dec);
            if (!elementDec) continue;
            return elementDec->contentType().str().toUpper() == "CDATA";
        }
        return false;
    }
    virtual bool closeOptional(const QString& elementName) const {
        if (isNull()) return false;
        DUChainReadLocker lock;
        QList<Declaration *> list = findDeclarations(elementName);
        foreach(Declaration * dec, list) {
            if (!dec) continue;
            ElementDeclaration *elementDec = dynamic_cast<ElementDeclaration *>(dec);
            if (!elementDec) continue;
            return !elementDec->closeTagRequired();
        }
        return false;
    }

    virtual bool emptyElement(const QString& elementName) const {
        if (isNull()) return false;
        DUChainReadLocker lock;
        QList<Declaration *> list = findDeclarations(elementName);
        foreach(Declaration * dec, list) {
            if (!dec) continue;
            ElementDeclaration *elementDec = dynamic_cast<ElementDeclaration *>(dec);
            if (!elementDec) continue;
            return elementDec->contentType().str().toUpper() == "EMPTY";
        }
        return false;
    }

    virtual bool hasChild(const QString& elementName, const QString& child) const {
        if (isNull()) return true;
        DUChainReadLocker lock;
        QList<Declaration *> list = findDeclarations(elementName);
        foreach(Declaration * dec, list) {
            if (!dec) continue;
            if (dec->kind() == Declaration::Type && dec->internalContext()) {
                QList<Declaration *> children;
                children.append(dec->internalContext()->findDeclarations(QualifiedIdentifier(child.toLower()), CursorInRevision::invalid(), AbstractType::Ptr()));
                if (children.size() > 0)
                    return true;
                else
                    return false;
            }
        }
        return true;
    }

protected:
    QList<Declaration *> findDeclarations(QString elementName) const {
        QList<Declaration *> list;
        list.append(topContext.data()->findDeclarations(QualifiedIdentifier(elementName.toLower()), CursorInRevision::invalid(), AbstractType::Ptr()));
        return list;
    }
    IndexedTopDUContext topContext;
};

//<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN" "http://www.w3.org/TR/html4/loose.dtd">
class Html4LooseHelper : public DtdHelperData {
public:
    Html4LooseHelper() : DtdHelperData() {
    }
    virtual ~Html4LooseHelper() {}

    virtual bool isNull() const {
        return false;
    }

    virtual bool cdataElement(const QString& elementName) const {
        static QHash<QString, char> list = ( {
            QHash<QString, char> list;
            list.insert("script", 0);
            list.insert("style", 0);
            list;
        });
        return list.contains(elementName.toLower());
    }

    virtual bool closeOptional(const QString& elementName) const {
        static QHash<QString, char> list = split("basefont | br | area | link | img | param | hr | p | dt | dd | li | input | option | thead | tbody | tfoot | colgroup | col | tr | th | td | frame | isindex | base | meta");
        return list.contains(elementName.toLower());
    }

    virtual bool emptyElement(const QString& elementName) const {
        static QHash<QString, char> list =  split("basefont | br | area | link | img | param | hr | input | col | frame | isindex | base | meta");
        return list.contains(elementName.toLower());
    }

    //close optional and not empty
    virtual bool hasChild(const QString& elementName, const QString& child) const {
        static QHash<QString, QHash<QString, char> > list = ( {
            QHash<QString, QHash<QString, char> > list;

            QHash<QString, char> inlineElements = split("tt | i | b | u | s | strike | big | small | em | strong | dfn | code | samp | kbd | var | cite | abbr | acronym | a | img | applet | object | font | basefont | br | script | map | q | sub | sup | span | bdo | iframe | input | select | textarea | label | button | #pcdata | cdata");

            QHash<QString, char> flowElements = split("p | h1 | h2 | h3 | h4 | h5 | h6 | ul | ol |  dir | menu | pre | dl | div | center | noscript | noframes | blockquote | form | isindex | hr | table | fieldset | address | #pcdata | cdata");
            flowElements.unite(inlineElements);

            list.insert("p", inlineElements);
            list.insert("dt", inlineElements);
            list.insert("dd", flowElements);
            list.insert("li", flowElements);
            list.insert("option", split(""));
            list.insert("thead", split("tr"));
            list.insert("tbody", split("tr"));
            list.insert("tfoot", split("tr"));
            list.insert("colgroup", split("col"));
            list.insert("tr", split("th | td"));
            list.insert("th", flowElements);
            list.insert("td", flowElements);
            list;
        });
        if (list.contains(elementName.toLower())) {
            return list[elementName.toLower()].contains(child.toLower());
        }
        return true;
    }

protected:
    QHash<QString, char> split(const QString &str) const {
        QHash<QString, char> ret;
        QStringList list = str.split("|");
        foreach(QString s, list) {
            ret.insert(s.trimmed().toLower(), 0);
        }
        return ret;
    }
};

//<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01//EN" "http://www.w3.org/TR/html4/strict.dtd">
class Html4StrictHelper : public Html4LooseHelper {
public:
    Html4StrictHelper() : Html4LooseHelper() {
    }
    virtual ~Html4StrictHelper() {}
};

//<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Frameset//EN" "http://www.w3.org/TR/html4/frameset.dtd">
class Html4FrameHelper : public Html4LooseHelper {
public:
    Html4FrameHelper() : Html4LooseHelper() {
    }
    virtual ~Html4FrameHelper() {}
};


class XmlHelper : public DtdHelperData {
public:
    XmlHelper() : DtdHelperData() {
    }
    virtual ~XmlHelper() {}

    virtual bool cdataElement(const QString& elementName) const {
        Q_UNUSED(elementName);
        //All cdata must be in <![CDATA[
        return false;
    }

    virtual bool closeOptional(const QString& elementName) const {
        Q_UNUSED(elementName);
        //Does not exist in XML
        return false;
    }

    virtual bool emptyElement(const QString& elementName) const {
        Q_UNUSED(elementName);
        //Safe to asume false
        return false;
    }

    virtual bool hasChild(const QString& elementName, const QString& child) const {
        Q_UNUSED(elementName);
        Q_UNUSED(child);
        //Should never be called because xml have no closeOptional elements.
        return true;
    }
    virtual bool isNull() const {
        return false;
    }
};

static DtdHelper html4LooseHelper(new Html4LooseHelper);
static DtdHelper html4StrictHelper(new Html4StrictHelper);
static DtdHelper html4FrameHelper(new Html4FrameHelper);
static DtdHelper xmlHelper(new XmlHelper);

DtdHelper DtdHelper::instance(const QString& publicId, const QString& systemId, const QString& URI, const QString& doctype, const KMimeType::Ptr mime, const KUrl& path)
{
    QString pubId = PublicId::decodeURN(publicId);

    if ((!pubId.isNull() && pubId == "-//W3C//DTD HTML 4.01//EN") ||
            (!pubId.isNull() && pubId == "-//W3C//DTD HTML 4.0//EN") ||
            (!systemId.isNull() && systemId == "http://www.w3.org/TR/html4/strict.dtd") ||
            (!systemId.isNull() && systemId == "http://www.w3.org/TR/REC-html40/strict.dtd")) {
        return html4StrictHelper;
    }

    if ((!pubId.isNull() && pubId == "-//W3C//DTD HTML 4.01 Transitional//EN") ||
            (!systemId.isNull() && systemId == "http://www.w3.org/TR/html4/loose.dtd")) {
        return html4LooseHelper;
    }

    if ((!pubId.isNull() && pubId == "-//W3C//DTD HTML 4.01 Frameset//EN") ||
            (!systemId.isNull() && systemId == "http://www.w3.org/TR/html4/frameset.dtd")) {
        return html4FrameHelper;
    }

    //Create a duchain helper if we can
    KUrl url = CatalogHelper::resolve(pubId, systemId, URI, doctype, mime, path);
    if (url.isValid()) {
        IndexedTopDUContext itop;
        {
            DUChainReadLocker lock;
            TopDUContext * top = DUChain::self()->chainForDocument(url);
            if (top)
                itop = top->indexed();
        }
        if (!itop.isValid()) {
            ILanguage *lang = ICore::self()->languageController()->language("Sgml");
            if (lang && lang->languageSupport()) {
                ParseJob * job = lang->languageSupport()->createParseJob(url);
                ThreadWeaver::Weaver::instance()->enqueue(job);
                //TODO Dodgy
                int count = 0;
                while (job && !job->isFinished() && ++count < 9999) {
                    usleep(1000);
                }
                {
                    DUChainReadLocker lock;
                    if (job && job->duChain().data())
                        itop = job->duChain().data()->indexed();
                }
                if (job) job->deleteLater();
            }
            return DtdHelper(new DUChainDTDHelper(itop));
        }
    }

    //Resort to failover
    
    DtdHelper helper = instanceForName(doctype);
    if (!helper.isNull())
        return helper;

    helper = instanceForMime(mime);
    if (!helper.isNull())
        return helper;

    return DtdHelper();
}


DtdHelper DtdHelper::instanceForMime(KMimeType::Ptr mime)
{
    if (!mime || !mime->isValid())
        return DtdHelper();

    if (mime->is("application/xml")
            || mime->is("text/xml")
            || mime->is("application/xslt+xml")
            || mime->is("application/wsdl+xml")
            || mime->is("application/x-wsdl")) {
        return xmlHelper;
    }

    if (mime->is("application/xhtml+xml")) {
        return xmlHelper;
    }

    if (mime->is("text/html")) {
        return html4LooseHelper;
    }

    return DtdHelper();
}

DtdHelper DtdHelper::instanceForName(const QString& name)
{
    if (name.trimmed().toLower() == "html") {
        return html4LooseHelper;
    }

    if (name.trimmed().toLower() == "xml") {
        return xmlHelper;
    }

    return DtdHelper();
}
