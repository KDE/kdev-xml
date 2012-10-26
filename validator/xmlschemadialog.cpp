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

#include "xmlschemadialog.h"

#include <QtGui/QVBoxLayout>
#include <QtGui/QLabel>
#include <QtGui/QComboBox>
#include <QtGui/QPushButton>

#include <KDE/KLocalizedString>
#include <KDE/KFileDialog>
#include <KDE/KUrlComboRequester>
#include <KDE/KComboBox>


XmlSchemaDialog::XmlSchemaDialog ( QWidget* parent, Qt::WindowFlags f ) : KDialog ( parent, f ) {
    QWidget * w = new QWidget();
    QVBoxLayout * vlayout = new QVBoxLayout();
    //QHBoxLayout * hlayout = new QHBoxLayout();
    KComboBox *comboBox = new KComboBox();
    comboBox->setEditable(true);
    m_comboBox = new KUrlRequester(comboBox, 0);
    m_comboBox->setFilter(i18n("*.xsd *.XSD *.dtd *.DTD|DTD and XML Schema Documents\n*|All Files"));
    vlayout->addWidget ( new QLabel ( i18n ( "Select/Enter a XML schema URL, DTD URL or DOCTYPE:" ) ) );
    vlayout->addWidget(m_comboBox);
    vlayout->addStretch(100);
    w->setLayout ( vlayout );
    setMainWidget ( w );
    setMaximumWidth(500);
    setMaximumHeight(300);
    setWindowTitle(i18n("Validate XML"));
}

XmlSchemaDialog::~XmlSchemaDialog() {}

void XmlSchemaDialog::setItems ( const QHash<QString, QString> &items ) {
    m_items = items;
    m_comboBox->clear();
    m_comboBox->comboBox()->clear();
    foreach ( QString s, items.keys() ) {
        m_comboBox->comboBox()->addUrl( s );
    }
}

QPair<QString, QString> XmlSchemaDialog::getItem() const {
    return QPair<QString, QString>(m_comboBox->text(), m_items[m_comboBox->text()]);
}


