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


#include "xmlvalidator.h"

#include <libxml/xmlschemas.h>
#include <libxml/catalog.h>
#include <libxml/parserInternals.h>
#include <stdio.h>
#include <stdarg.h>

#include <QtCore/QDebug>

#include <KDE/KUrl>

#include <xmlcatalog/icatalogmanager.h>
#include <xmlcatalog/idocumentcachemanager.h>
#include <KDE/KLocalizedString>
#include <KDE/KDebug>

#define debug() kDebug()

int XmlValidator::m_refCount = 0;


xmlExternalEntityLoader defaultLoader = NULL;

xmlParserInputPtr catalogManagerEntityLoader(const char *URL, const char *ID, xmlParserCtxtPtr ctxt) {
    xmlParserInputPtr ret;
    QString resolved = ICatalogManager::self()->resolve(ID, URL);
    if(resolved.isEmpty())
        resolved = ICatalogManager::self()->resolveUri(URL);
    if(resolved.isEmpty())
        resolved = URL;
    resolved = IDocumentCacheManager::self()->cachedUrl(resolved);
    debug() << "Resolved" << URL << ID << " to " << resolved;
    if(resolved.isEmpty()) {
        debug() << "Unable to locate file for" << URL << ID;
        return NULL;
    }
    ret = xmlNewInputFromFile(ctxt, resolved.toAscii().constData());
    if (ret != NULL)
        return(ret);
    if (defaultLoader != NULL)
        ret = defaultLoader(URL, ID, ctxt);
    return(ret);
}


XmlValidator::XmlValidator()
{
    if(m_refCount == 0) {
        if(!defaultLoader)
            defaultLoader = xmlGetExternalEntityLoader();
        xmlSetExternalEntityLoader(catalogManagerEntityLoader);
        //QStringList catalogs = ICatalogManager::self()->catalogFileList();
        //if(catalogs.length() > 0)
        //    xmlLoadCatalogs(catalogs.join(";").toLatin1().constData());
    }
    m_refCount++;
}

XmlValidator::~XmlValidator()
{
    m_refCount--;
    if(m_refCount == 0) {
        //xmlCatalogCleanup();
    }
}

void XmlValidator::errorMessage ( void* ctx, const char* msg, ... ) {
    if ( ctx == NULL || msg == NULL || strlen ( msg ) < 1 )
        return;
    XmlValidator *v = static_cast<XmlValidator *> ( ctx );
    va_list vl;
    va_start ( vl, msg );
    char errorStr[8192];
    vsprintf ( errorStr, msg, vl );
    v->m_errors.append ( errorStr );
    va_end ( vl );
    kDebug() << errorStr;
}

void XmlValidator::warnMessage ( void* ctx, const char* msg, ... ) {
    if ( ctx == NULL || msg == NULL )
        return;
    XmlValidator *v = static_cast<XmlValidator *> ( ctx );
    va_list vl;
    va_start ( vl, msg );
    char errorStr[8192];
    vsprintf ( errorStr, msg, vl );
    v->m_warnings.append ( errorStr );
    va_end ( vl );
    kDebug() << errorStr;
}


XmlValidator::ValidationResult XmlValidator::validateSchema ( const QString &documentUrl, const QString &schemaUrl ) {
    if ( documentUrl.isEmpty() || schemaUrl.isEmpty() )
        return InternalError;
    m_errors.clear();
    m_warnings.clear();

    int result = 0;

    xmlSchemaParserCtxtPtr parserCtxPtr = xmlSchemaNewParserCtxt ( KUrl ( schemaUrl ).toEncoded().constData() );
    if ( !parserCtxPtr ) {
        debug() << "Unable to create xmlSchemaParserCtxtPtr";
        return InternalError;
    }
    xmlSchemaValidityErrorFunc errorFunc = &XmlValidator::errorMessage;
    xmlSchemaValidityWarningFunc warnFunc = &XmlValidator::warnMessage;

    xmlSchemaSetParserErrors ( parserCtxPtr, errorFunc, warnFunc, this );

    xmlSchemaPtr schemaPtr = xmlSchemaParse ( parserCtxPtr );
    if ( !schemaPtr ) {
        xmlSchemaFreeParserCtxt ( parserCtxPtr );
        return InternalError;
    }

    xmlSchemaValidCtxtPtr validContextPtr = xmlSchemaNewValidCtxt ( schemaPtr );
    if ( !validContextPtr || xmlSchemaIsValid ( validContextPtr ) != 1 ) {
        xmlSchemaFree ( schemaPtr );
        xmlSchemaFreeParserCtxt ( parserCtxPtr );
        return InternalError;
    }

    xmlSchemaSetValidErrors ( validContextPtr, errorFunc, warnFunc, this );

    result = xmlSchemaValidateFile ( validContextPtr, KUrl ( documentUrl ).toEncoded().constData(), 0 );

    xmlSchemaFree ( schemaPtr );
    xmlSchemaFreeParserCtxt ( parserCtxPtr );
    xmlSchemaFreeValidCtxt ( validContextPtr );
    
    if ( result == -1 )
        return InternalError;
    else if ( result )
        return Failed;

    return Success;
}


XmlValidator::ValidationResult XmlValidator::validateDTD ( const QString& documentUrl, const QString& dtdUrl ) {
    if ( documentUrl.isEmpty() )
        return InternalError;
    m_errors.clear();
    m_warnings.clear();

    int result = 0;
    xmlDocPtr docPtr = 0;

    if ( !dtdUrl.isEmpty() ) {
        //TODO Add a handler for the parser errors.
        docPtr = xmlParseFile ( documentUrl.toLatin1().constData() );
        if ( !docPtr ) {
            return InternalError;
        }

        xmlValidityErrorFunc errorFunc = &XmlValidator::errorMessage;
        xmlValidityWarningFunc warnFunc = &XmlValidator::warnMessage;

        xmlValidCtxt cvp;
        cvp.error = errorFunc;
        cvp.warning  = warnFunc;
        cvp.userData = ( void * ) this;

        //TODO Add a handler for the parser errors.
        xmlDtdPtr dtdPtr = 0;
        dtdPtr = xmlParseDTD ( NULL, ( xmlChar * ) dtdUrl.toLatin1().constData() );
        if ( dtdPtr ) {
            result = xmlValidateDtd ( &cvp, docPtr, dtdPtr );
            xmlFreeDtd ( dtdPtr );
        }
    } else {
        xmlParserCtxtPtr ctxt;
        xmlDocPtr docPtr;
        ctxt = xmlNewParserCtxt();
        if ( !ctxt )
            return InternalError;
        docPtr = xmlCtxtReadFile ( ctxt, documentUrl.toLatin1().constData(), NULL, XML_PARSE_DTDVALID );
        result = ctxt->valid;
        m_errors.append ( QString ( "%1:%2 %3" ).arg ( ctxt->lastError.line ).arg ( ctxt->lastError.int2 ).arg ( ctxt->lastError.message ) );
        xmlFreeParserCtxt ( ctxt );
    }

    if ( docPtr )
        xmlFreeDoc ( docPtr );

    if ( result == -1 )
        return InternalError;
    else if ( !result )
        return Failed;
    return Success;
}

