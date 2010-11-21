/*****************************************************************************
 * Copyright (c) 2007 Andreas Pakulat <apaku@gmx.de>                         *
 * Copyright (c) 2007 Piyush verma <piyush.verma@gmail.com>                  *
 * Copyright (c) 2008-2009 Niko Sams <niko.sams@gmail.com>                   *
 * Copyright (c) 2010 Ruan Strydom <rm3dom@gmail.com>                        *
 *                                                                           *
 * Permission is hereby granted, free of charge, to any person obtaining     *
 * a copy of this software and associated documentation files (the           *
 * "Software"), to deal in the Software without restriction, including       *
 * without limitation the rights to use, copy, modify, merge, publish,       *
 * distribute, sublicense, and/or sell copies of the Software, and to        *
 * permit persons to whom the Software is furnished to do so, subject to     *
 * the following conditions:                                                 *
 *                                                                           *
 * The above copyright notice and this permission notice shall be            *
 * included in all copies or substantial portions of the Software.           *
 *                                                                           *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,           *
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF        *
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND                     *
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE    *
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION    *
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION     *
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.           *
 *****************************************************************************/
#ifndef XML_PARSESESSION_H
#define XML_PARSESESSION_H

#include <QtCore/QString>
#include <language/editor/simplecursor.h>
#include "sgmlparser.h"

namespace KDevPG
{
class MemoryPool;
}
namespace KDevelop
{
class SimpleCursor;
}
namespace Xml
{
class TokenStream;
class StartAst;

typedef QPair<KDevelop::DUContextPointer, KDevelop::SimpleRange> SimpleUse;

class KDEVSGMLPARSER_EXPORT ParseSession
{
public:
    ParseSession();
    ~ParseSession();

    void setContents(const QString& contents);
    void setCurrentDocument(const KDevelop::IndexedString& filename);
    bool readFile(const QString& filename, const char* charset = 0);
    void setDebug(bool);
    TokenStream* tokenStream() const;
    QString contents() const;

    bool parse(StartAst**);
    Parser* createParser();
    Parser* createParser(KMimeType::Ptr mime) {setMime(mime); return createParser();}

    QString symbol(qint64 token) const;
    
    QString symbol(AstNode* node) const;

    /**
     * Return the position (\a line%, \a column%) of the \a offset in the file.
     *
     * \note the line starts from 0.
     */
    KDevelop::CursorInRevision positionAt(qint64 offset) const;

    QList<KDevelop::ProblemPointer> problems();

    //TODO warning implement this
    void mapAstUse(AstNode* node, const SimpleUse& use)
    {
        Q_UNUSED(node);
        Q_UNUSED(use);
    }
    
    ///Tells the parser to use DTD or SGML
    ///If this is not set the parser will guess the mime using the document URL and/or content.
    void setMime(KMimeType::Ptr mime) { m_mime = mime; }

private:
    QString m_contents;
    bool m_debug;
    KDevelop::IndexedString m_currentDocument;
    KDevPG::MemoryPool* m_pool;
    TokenStream* m_tokenStream;
    QList<KDevelop::ProblemPointer> m_problems;
    KMimeType::Ptr m_mime;
};

}

#endif

// kate: space-indent on; indent-width 4; tab-width 4; replace-tabs on; auto-insert-doxygen on
