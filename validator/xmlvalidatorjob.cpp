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

#include "xmlvalidatorjob.h"

#include <QtCore/QFile>

#include <KDE/KUrl>
#include <KDE/KMessageBox>
#include <KDE/KLocalizedString>

#include <xmlcatalog/icatalogmanager.h>
#include <xmlcatalog/idocumentcachemanager.h>


XmlValidatorJob::XmlValidatorJob(QObject* parent): KJob(parent)
{
    connect(this, SIGNAL(signalReady(KJob*)), this, SLOT(ready(KJob*)));
}

XmlValidatorJob::~XmlValidatorJob()
{}

XmlValidatorJob* XmlValidatorJob::dtdValidationJob(const QString &document, const QString &dtd)
{
    XmlValidatorJob *job = new XmlValidatorJob();
    job->m_dtdUrl = dtd;
    job->m_documentUrl = document;
    job->setObjectName(i18n("Validating document: %1", document));
    return job;
}

XmlValidatorJob* XmlValidatorJob::schemaValidationJob(const QString &document, const QString &schema)
{
    XmlValidatorJob *job = new XmlValidatorJob();
    job->m_schemaUrl = schema;
    job->m_documentUrl = document;
    job->setObjectName(i18n("Validating document: %1", document));
    return job;
}

void XmlValidatorJob::start()
{
    if(m_documentUrl.isEmpty()) {
        m_result = Failed;
        m_errors.append(i18n("No document to validate"));
        setError(m_result);
        emitResult();
        return;
    }

    
    QString localUrl;
    KJob *copyJob = 0;

    //DTD inline
    if(m_dtdUrl.isEmpty() && m_schemaUrl.isEmpty()) {
        emit signalReady(this);
        return;
    }
    
    if(!m_dtdUrl.isEmpty()) {
        localUrl = getLocalURLForSchema(m_documentUrl, m_dtdUrl);
        if(QFile::exists(localUrl)) {
            m_dtdUrl = localUrl;
            emit signalReady(this);
            return;
        } else {
            copyJob = KIO::copy(m_dtdUrl, localUrl, KIO::HideProgressInfo);
            m_dtdUrl = localUrl;
        }
    }
    
    if(!m_schemaUrl.isEmpty()) {
        localUrl = getLocalURLForSchema(m_documentUrl, m_schemaUrl);
        if(QFile::exists(localUrl)) {
            m_schemaUrl = localUrl;
            emit signalReady(this);
            return;
        } else {
            copyJob = KIO::copy(m_schemaUrl, localUrl, KIO::HideProgressInfo);
            m_schemaUrl = localUrl;
        }
    }

    copyJob->setAutoDelete(true);
    copyJob->setUiDelegate(0);
    connect(copyJob, SIGNAL(result(KJob *)), this, SLOT(ready(KJob *)));
    copyJob->start();
}

void XmlValidatorJob::validate()
{
    if(!m_schemaUrl.isEmpty() && !m_documentUrl.isEmpty())
        m_result = validateSchema(m_documentUrl, m_schemaUrl);
    else if(!m_documentUrl.isEmpty())
        m_result = validateDTD(m_documentUrl, m_dtdUrl);
    setError(m_result);
    emitResult();
}

void XmlValidatorJob::ready(KJob* job)
{
    if(!job)
        return;
    if(job->error() > 0) {
        if(job != this) {
            m_result = Failed;
            setError(m_result);
            m_errors = job->errorString().split("\n");
        }
        emitResult();
        return;
    }
    validate();
}

QString XmlValidatorJob::getLocalURLForSchema ( const QString& file, const QString& schema ) {
    if ( file.isEmpty() || schema.isEmpty() )
        return QString();
    KUrl schemaUrl(schema);
    KUrl fileUrl(file);
    if ( schemaUrl.isRelative() ) {
        QString schemaDir = schemaUrl.directory ( KUrl::IgnoreTrailingSlash );
        QString localFile;
        if ( !schemaDir.isEmpty() )
            localFile =  QString ( "%1/%2/%3" ).arg ( fileUrl.directory (), schemaDir, schemaUrl.fileName() );
        else
            localFile = QString ( "%1/%2" ).arg ( fileUrl.directory (), schemaUrl.fileName() );
        if (QFile::exists(localFile))
            return localFile;
    }

    QString url = IDocumentCacheManager::self()->getLocalUrl(schema);
    return url;
}


QString XmlValidatorJob::errorString() const
{
    QString ret = errors().join("\n");
    if(warnings().size() > 0)
        ret += "\nWarnings:\n" + warnings().join("\n");
    return ret;
}


#include "xmlvalidatorjob.moc"
