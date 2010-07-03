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

#include "formatterpreferences.h"
#include "formatter.h"
#include "sgmlformatter.h"

#include <KDE/KDebug>

using namespace Xml;

FormatterPreferences::FormatterPreferences ( QWidget* parent, Qt::WindowFlags f )
        : KDevelop::SettingsWidget ( parent ) {
    Q_UNUSED ( f );
    setupUi ( this );
    connect ( spacingSbx, SIGNAL ( valueChanged ( int ) ), this, SLOT ( indentChanged ( int ) ) );
    connect ( caseCbx, SIGNAL ( currentIndexChanged(int)), this, SLOT ( caseChanged ( int ) ) );
    connect ( formatSourceCbx, SIGNAL (stateChanged(int)), this, SLOT (formatSourceChanged ( int ) ) );
    m_formatter = new Xml::SgmlFormatter;
}

FormatterPreferences::~FormatterPreferences() {
    if ( m_formatter )
        delete m_formatter;
}

void FormatterPreferences::load(const KDevelop::SourceFormatterStyle& style)
{
    spacingSbx->blockSignals(true);
    caseCbx->blockSignals(true);
    
    m_formatter->loadStyle ( style.content() );
    int indent = getOptions()["INDENT"].toInt();
    if ( indent < 0 || indent > 99 )
      indent = 4;
    spacingSbx->setValue(indent);
    if(getOptions()["CASE"].toString() == "LOWER")
        caseCbx->setCurrentIndex(1);
    else if(getOptions()["CASE"].toString() == "UPPER")
        caseCbx->setCurrentIndex(2);
    else
        caseCbx->setCurrentIndex(0);

    spacingSbx->blockSignals(false);
    caseCbx->blockSignals(false);
}

QString FormatterPreferences::save() {
    return m_formatter->saveStyle();
}

QMap< QString, QVariant > FormatterPreferences::getOptions() {
    return m_formatter->options();
}

void FormatterPreferences::caseChanged ( int i )
{
    QString caseSetting;
    if( i == 1)
        caseSetting = "LOWER";
    else if( i == 2)
        caseSetting = "UPPER";
    else
        caseSetting = "NONE";

    m_formatter->options()["CASE"] = caseSetting;
    updateText ( );
}

void FormatterPreferences::indentChanged ( int i )
{
    m_formatter->options()["INDENT"] = i;
    updateText ( );
}

void FormatterPreferences::formatSourceChanged(int i)
{
    m_formatter->options()["SOURCES"] = formatSourceCbx->isChecked();

    updateText ( );
}

void FormatterPreferences::updateText ( ) {
    QString text = QString ( "<?xml version='1.0' encoding='UTF-8'?>\n" ) +
    "<Quote type=\"virtual\" >\n" +
    "  <ContactAboutQuote value=\"true\" />\n" +
    "  <SendMarketingMaterial value=\"false\" />\n" +
    "  <DisplayCommission value=\"false\" />\n" +
    "  <RequestingEntity entityId=\"1051920013\" >\n" +
    "    <RequestingEntityAttribute attribute=\"Name\" >Peterpan</RequestingEntityAttribute>\n" +
    "  </RequestingEntity>\n" +
    "</Quote>\n";
    emit previewTextChanged ( m_formatter->formatSource ( text ) );
}

#include "formatterpreferences.moc"
