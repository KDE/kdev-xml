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

#include "xmlformatter.h"

#include <KDE/KDebug>

#include <interfaces/isourceformatter.h>

using namespace Xml;

XmlFormatter::XmlFormatter() : SgmlFormatter() {
}


QString XmlFormatter::formatSource ( const QString& text, const QString& leftContext, const QString& rightContext ) {
    Q_UNUSED ( rightContext );
    kDebug();

    if ( text.size() < 1 )
        return text;

    int indent = m_options["INDENT"].toInt();
    if ( indent < 0 || indent > 99 )
        indent = 4;

    QString strResult = "";
    QTextStream result ( &strResult );
    QString strIn = text;
    QTextStream in ( &strIn );

    TokenType type;
    QStringRef token;
    QString content;
    bool inElement = false;
    int depth = 0;
    QString endLine = "\n";
    //This is for a newline quirk
    int formatCount = 0;

    m_contentDataIndex = 0;
    m_lineCount = 0;
    m_dtdHelper = DtdHelper::instanceForName("xml");


    if ( leftContext.size()  > 0 ) {
        m_content = leftContext;
        m_contentData = leftContext.data();
        m_contentDataLength = leftContext.length();

        while ( m_contentDataIndex <  m_contentDataLength) {
            token = nextToken ( (int *)&type );
            switch ( type ) {
            case  StartElement:
                depth++;
                inElement = true;
                break;
            case  EndElement:
                depth--;
                break;
            case  ClosedElement:
                inElement = false;
                break;
            case  Error:
                kDebug() << "Error in document: " << token.toString();
                return text;
                break;
            };
        }
    }

    m_content = text;
    m_contentData = text.data();
    m_contentDataLength = text.length();
    m_contentDataIndex = 0;

    while ( m_contentDataIndex <  m_contentDataLength) {
        token = nextToken ( (int*)&type );

        switch ( type ) {
        case  Error:
            kDebug() << "Error in document: " << token.toString();
            return text;
            break;
        case  Processing:
            result << content << token.toString();
            content = "";
            formatCount++;
            break;
        case  ClosedElement: {
            //NOTE Duplication of below
            int align = 0;
            QStringList list;
            if ( content.size() > 0 ) {
                list = content.split ( endLine );
                for ( int i = 0; i < list.size() - 1; i++ )
                    result << list[i] << endLine;
                if ( !list.last().trimmed().isEmpty() )
                    result << list.last() << endLine;
                if ( list.size() > 0 )
                    align = list.last().size();
            } else if (formatCount)
                result << endLine;
            content = "";
            list = token.toString().split ( endLine );
            result << indentString ( depth, indent ) << list.first();
            for ( int i = 1; i < list.size(); i++ )
                result << endLine << indentString ( depth, indent ).remove ( 0, align ) << list[i];

            formatCount++;
            inElement = false;
        }
        break;
        case  StartElement: {
            //This needs to keep parameters that break alignment.
            int align = 0;
            QStringList list;
            if ( content.size() > 0 ) {
                list = content.split ( endLine );
                for ( int i = 0; i < list.size() - 1; i++ )
                    result << list[i] << endLine;
                if ( !list.last().trimmed().isEmpty() )
                    result << list.last() << endLine;
                if ( list.size() > 0 )
                    align = list.last().size();
            } else if (formatCount)
                result << endLine;
            content = "";
            list = token.toString().split ( endLine );
            result << indentString ( depth, indent ) << list.first();
            for ( int i = 1; i < list.size(); i++ )
                result << endLine << indentString ( depth, indent ).remove ( 0, align ) << list[i];

            depth++;
            formatCount++;
            inElement = true;
        }
        break;
        case  EndElement:
            if ( inElement ) {
                result << content << indentString( depth - 1, indent ) << token.toString();
            } else {
                if ( content.trimmed().size() > 0 ) {
                    QStringList list = content.split ( endLine );
                    for ( int i = 0; i < list.size() - 1; i++ )
                        result << list[i] << endLine;
                    if ( !list.last().trimmed().isEmpty() ) {
                        result << list.last() << endLine;
                    }
                } else
                    result << endLine;
                result << indentString ( depth - 1, indent ) << token.toString();
            }
            inElement = false;
            depth--;
            content = "";
            formatCount++;
            break;
        case  DTD:
            result << content << token.toString();
            content = "";
            formatCount++;
            break;
        case  Comment:
            result << content << token.toString();
            content = "";
            formatCount++;
            break;
        case  Text:
            content += token.toString();
            formatCount++;
            break;
        default:
            kDebug() << "Error in document: " << token.toString();
            return text;
        };
    }
    if ( content.length() > 0 )
        result << content;
    return strResult;
}

QString XmlFormatter::compactSource ( const QString& text ) {
    if ( text.size() < 1 )
        return text;

    QString strResult = "";
    QTextStream result ( &strResult );
    QString strIn = text;
    QTextStream in ( &strIn );

    TokenType type;
    QString token;
    QString content;
    bool inElement = false;
    int depth = 0;
    QString endLine = "\n";

    while ( m_contentDataIndex <  m_contentDataLength) {
        token = nextToken ( (int*)&type ).toString();
        switch ( type ) {
        case  Error:
            kDebug() << "Error in document: " << token;
            return text;
            break;
        case  Processing:
            result << token.trimmed();
            content = "";
            break;
        case  ClosedElement: {
            QStringList list;
            result << content.trimmed();
            content = "";
            list = token.split ( endLine );
            result << list.first().trimmed();
            for ( int i = 1; i < list.size(); i++ )
                result << " " << list[i].trimmed();
            inElement = false;
        }
        break;
        case  StartElement: {
            QStringList list;
            result << content.trimmed();
            content = "";
            list = token.split ( endLine );
            result << list.first().trimmed();
            for ( int i = 1; i < list.size(); i++ )
                result << " " << list[i].trimmed();
            depth++;
            inElement = true;
        }
        break;
        case  EndElement:
            //Not a text-element
            if ( !inElement ) {
                result << content.trimmed();
                content = "";
            }
            inElement = false;
            depth--;
            result << content << token;
            content = "";
            break;
        case  DTD:
            result << content.trimmed() << token;
            content = "";
            break;
        case  Comment:
            result << content.trimmed() << token;
            content = "";
            break;
        case Other:
            result << content.trimmed() << token;
        case  Text:
            content += token;
            break;
        default:
            kDebug() << "Error in document: " << token;
            return text;
        };
    }
    if ( content.length() > 0 )
        result << content;
    return strResult;
}
