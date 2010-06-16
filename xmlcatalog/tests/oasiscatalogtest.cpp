/*****************************************************************************
 * Copyright (c) 2009 Ruan Strydom <rm3dom@gmail.com>                        *
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

#include <QtTest>
#include <QtCore>
#include <icatalogmanager.h>
#include <oasiscatalog.h>
#include <oasiscatalogreaderwriter.h>
#include <oasiscatalog.h>
#include <publicid.h>
#include <KDebug>

class testOASISCatalog : public QObject {
    Q_OBJECT
    ICatalog * read();
private slots:
    void testManager();
    void testReader();
    void testAddEntry();
    void testURN();
    void testResolve();
    void testResolvePublic();
    void testResolveSystem();
    void testRewriteSystem();
    void testSystemSuffix();
    void testResolveUri();
    void testGroupBase();
    //void testDelegatePublic() {  }
    //void testDelegateSystem() {  }
    //void testDelegateUri() {  }
};


ICatalog* testOASISCatalog::read() {
    QString catalog = "<catalog prefer=\"public\">\
    <public publicId=\"-//OASIS//DTD XML Exchange Table Model 19990315//EN\" uri=\"soextblx.dtd\" />\
    <public publicId=\"-//OASIS//DTD DocBook MathML Module V1.0//EN\" uri=\"http://www.oasis-open.org/docbook/xml/mathml/1.0/dbmathml.dtd\"/>\
    <system systemId=\"http://www.oasis-open.org/docbook/xml/4.1.2/docbookx.dtd\" uri=\"docbookx.dtd\"/>\
    <rewriteSystem systemIdStartString=\"http://www.oasis-open.org/committees/\" rewritePrefix=\"file:///projects/oasis/\"/>\
    <group xml:base=\"file:///projects/oasis/\">\
      <public publicId=\"-//OASIS//DTD DocBook XML V4.1.2//EN\" uri=\"docbookx.dtd\"/>\
      <systemSuffix systemIdSuffix=\"docbookx.dtd\" uri=\"file:///projects/oasis/xml/4.4/docbookx.dtd\"/>\
      <systemSuffix systemIdSuffix=\"4.3/docbookx.dtd\" uri=\"file:///projects/oasis/xml/4.3/docbookx.dtd\"/>\
    </group>\
    <uri name=\"http://www.oasis-open.org/committees/docbook/\" uri=\"file:///projects/oasis/docbook/website/\"/>\
    </catalog>";
    QDomDocument d;
    d.setContent(catalog,true);
    OASISCatalogReaderWriter rw;
    return rw.readCatalog(d);
}

void testOASISCatalog::testURN() {
    ICatalog * c = read();
    if(!c)
        QFAIL("FAILED READING - RETURNED NULL");
    QString m = "http://www.oasis-open.org/docbook/xml/mathml/1.0/dbmathml.dtd";
    QString s = c->resolvePublicId(PublicId::encodeURN("-//OASIS//DTD DocBook MathML Module V1.0//EN"));
    if( s != m)
        QFAIL(QString("EXPECTED %1 GOT %2").arg(m,s).toAscii().constData());
}

void testOASISCatalog::testManager()
{
    ICatalogManager::self()->addCatalog("/home/ruan/kdevelop/share/apps/kdevxmlcatalog/catalogs/HTML.xml");
    kDebug() << ICatalogManager::self()->resolveDoctype("html");
    kDebug() << ICatalogManager::self()->resolve("-//W3C//DTD HTML 4.01 Transitional//EN", QString());
}

void testOASISCatalog::testAddEntry()
{
    QString catalog = "<catalog prefer=\"public\">\
    </catalog>";
    QDomDocument d;
    d.setContent(catalog, true);
    OASISCatalogReaderWriter rw;
    ICatalog *c = rw.readCatalog(d);
    c->setParameter("xml:base", "file:///tmp/");
    QString systemId = "http://www.oasis-open.org/docbook/xml/4.1.2/docbookx.dtd";
    c->addSystemEntry(systemId, "docbookx.dtd", QHash<QString, QVariant>());
    QString m = "file:///tmp/docbookx.dtd";
    QString s = c->resolveSystemId(systemId);
    if( s != m)
        QFAIL(QString("EXPECTED %1 GOT %2").arg(m,s).toAscii().constData());
}


void testOASISCatalog::testReader() {
    ICatalog * c = read();
    if(!c)
        QFAIL("FAILED READING - RETURNED NULL");
}

void testOASISCatalog::testResolve()
{
    ICatalog * c = read();
    if(!c)
        QFAIL("FAILED READING - RETURNED NULL");
    QString m = "soextblx.dtd";
    QString s = c->resolve("-//OASIS//DTD XML Exchange Table Model 19990315//EN", "soextblx.dtd");
    if( s != m)
        QFAIL(QString("EXPECTED %1 GOT %2").arg(m,s).toAscii().constData());
}


void testOASISCatalog::testResolvePublic() {
    ICatalog * c = read();
    if(!c)
        QFAIL("FAILED READING - RETURNED NULL");
    QString m = "http://www.oasis-open.org/docbook/xml/mathml/1.0/dbmathml.dtd";
    QString s = c->resolvePublicId("-//OASIS//DTD DocBook MathML Module V1.0//EN");
    if( s != m)
        QFAIL(QString("EXPECTED %1 GOT %2").arg(m,s).toAscii().constData());
}

void testOASISCatalog::testResolveSystem() {
    ICatalog * c = read();
    if(!c)
        QFAIL("FAILED READING - RETURNED NULL");
    QString m = "docbookx.dtd";
    QString s = c->resolveSystemId("http://www.oasis-open.org/docbook/xml/4.1.2/docbookx.dtd");
    if( s != m)
        QFAIL(QString("EXPECTED %1 GOT %2").arg(m,s).toAscii().constData());
}


void testOASISCatalog::testRewriteSystem() {
    ICatalog * c = read();
    if(!c)
        QFAIL("FAILED READING - RETURNED NULL");

    QString s = c->resolveSystemId("http://www.oasis-open.org/committees/committees.xsd");
    QString m = "file:///projects/oasis/committees.xsd";
    if( s != m)
        QFAIL(QString("EXPECTED %1 GOT %2").arg(m,s).toAscii().constData());
}

void testOASISCatalog::testGroupBase()
{
    ICatalog * c = read();
    if(!c)
        QFAIL("FAILED READING - RETURNED NULL");
    
    QString s = c->resolvePublicId("-//OASIS//DTD DocBook XML V4.1.2//EN");
    QString m = "file:///projects/oasis/docbookx.dtd";
    if( s != m)
        QFAIL(QString("EXPECTED %1 GOT %2").arg(m,s).toAscii().constData());
}

void testOASISCatalog::testSystemSuffix()
{
    ICatalog * c = read();
    if(!c)
        QFAIL("FAILED READING - RETURNED NULL");
    
    QString s = c->resolveSystemId("http://www.oasis-open.org/docbook/xml/4.3/docbookx.dtd");
    QString m = "file:///projects/oasis/xml/4.3/docbookx.dtd";
    if( s != m)
        QFAIL(QString("EXPECTED %1 GOT %2").arg(m,s).toAscii().constData());

    s = c->resolveSystemId("http://www.oasis-open.org/docbook/xml/4/docbookx.dtd");
    m = "file:///projects/oasis/xml/4.4/docbookx.dtd";
    if( s != m)
        QFAIL(QString("EXPECTED %1 GOT %2").arg(m,s).toAscii().constData());
}

void testOASISCatalog::testResolveUri()
{
    ICatalog * c = read();
    if(!c)
        QFAIL("FAILED READING - RETURNED NULL");
    
    QString s = c->resolveUri("http://www.oasis-open.org/committees/docbook/");
    QString m = "file:///projects/oasis/docbook/website/";
    if( s != m)
        QFAIL(QString("EXPECTED %1 GOT %2").arg(m,s).toAscii().constData());
}

QTEST_MAIN ( testOASISCatalog )
#include "xmlcatalogtest.moc"

