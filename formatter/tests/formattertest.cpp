#include "formattertest.h"

#include <QTest>

#include "../sgmlformatter.h"
#include "../xmlformatter.h"
#include <KDebug>
#include <htmlformatter.h>
#include <QFile>

QTEST_MAIN(Xml::TestFormatter)

Xml::TestFormatter::TestFormatter()
{

}

void Xml::TestFormatter::testHtml()
{
    /*
    QFile f("/media/usb_large/tests/html/yahoo.html");
    f.open(QIODevice::ReadOnly);
    QTextStream stream(&f);
    QString test = stream.readAll();
    kDebug() << test.length();
    //QString test = "<body>\n text <?php \n blah \n ?>\n</body>";
    Xml::HtmlFormatter formatter;
    //formatter.setDebugEnabled(true);
    test = formatter.formatSource(test);
    kDebug() << test.length();
    */
}

void Xml::TestFormatter::testXml_data()
{
  QTest::addColumn<QString>("leftCtx");
  QTest::addColumn<QString>("code");
  QTest::addColumn<QString>("rightCtx");
  QTest::addColumn<QString>("expected");

  QTest::newRow("xml preamble") << QString("  ") << QString("<?xml version=\"1.0\" encoding=\"utf-8\"?>") << QString("")
                                << QString("<?xml version=\"1.0\" encoding=\"utf-8\"?>");

  QTest::newRow("first") << QString("<?xml version=\"1.0\" encoding=\"utf-8\"?>") << QString("<outer>") <<  QString("")
                                << QString("<outer>");

  QTest::newRow("nested") << QString("<?xml version=\"1.0\" encoding=\"utf-8\"?><outer>") << QString("<inner>") <<  QString("")
                                << QString("  <inner>");

  QTest::newRow("nested2") << QString("<?xml version=\"1.0\" encoding=\"utf-8\"?><outer><inner>") << QString("<inner2>") <<  QString("")
                                << QString("    <inner2>");

  QTest::newRow("second") << QString("<?xml version=\"1.0\" encoding=\"utf-8\"?><outer><inner></inner>") << QString("<inner>") <<  QString("")
                                << QString("  <inner>");

  QTest::newRow("closeTag") << QString("<open>\n  <inner>\n") << QString("</inner>") << QString("") << QString("  </inner>");
}

void Xml::TestFormatter::testXml()
{
  QFETCH(QString, leftCtx);
  QFETCH(QString, code);
  QFETCH(QString, rightCtx);
  QFETCH(QString, expected);

  XmlFormatter formatter;
  formatter.options()["INDENT"] = 2;

  QString actual = formatter.formatSource(code, leftCtx, rightCtx);
  QCOMPARE(actual, expected);
}

#include "formattertest.moc"
