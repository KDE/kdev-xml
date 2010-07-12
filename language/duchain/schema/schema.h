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

#ifndef SCHEMA_H
#define SCHEMA_H

/* NOTE TODO
 * Another quick and dirty implementation.
 * This keeps a list of elements their attributes and enums in no particular order.
 * This is extremely elementry most of the work is done by the SchemaBuilder.
 * As with the DTD parser this needs to be able to differentiate between choice, sequence etc.
 * Validation is left to the validation plugin for now.
 */

#include <QtCore/QList>
#include <KDE/KSharedPtr>
#include <KDE/KUrl>

#include <language/duchain/indexedstring.h>
#include "duchainexport.h"


/* Only a 30000 foot view of a schema document */
namespace Xml {
class SchemaQName;
class SchemaNode;
class SchemaSimpleType;
class SchemaComplexType;
class SchemaEnum;
class SchemaAttribute;
class SchemaElement;
class SchemaAttributeGroup;
class SchemaGroup;
class Schema;
class SchemaController;
class SchemaMutexLocker;
}


uint KDEVSGMLDUCHAIN_EXPORT qHash(const Xml::SchemaQName &node);


namespace Xml {

typedef KSharedPtr<SchemaNode> SchemaNodePtr;


class KDEVSGMLDUCHAIN_EXPORT SchemaQName {
public:
    SchemaQName(const KDevelop::IndexedString &_name, const KDevelop::IndexedString &_ns) : name(_name), ns(_ns) {}

    KDevelop::IndexedString name;

    KDevelop::IndexedString ns;

    inline bool operator == (const SchemaQName &rhs) const {
        return name == rhs.name && ns == rhs.ns;
    }

    inline bool operator < (const SchemaQName &rhs) const {
        if (ns == rhs.ns)
            return name < rhs.name;
        if (ns < rhs.ns)
            return true;
        return name < rhs.name;
    }
};


/** Index nodes by identifiers */
class KDEVSGMLDUCHAIN_EXPORT SchemaNodeMap {
public:
    SchemaNodeMap() {}
    virtual ~SchemaNodeMap() {}

    virtual inline void insert(SchemaNodePtr &node);

    virtual inline QList<SchemaNodePtr> values() const {
        return m_uniqueMap.values();
    }

    virtual inline QList<SchemaQName> keys() const {
        return m_uniqueMap.keys();
    }

    virtual inline const SchemaNodePtr value(const SchemaQName &id) const {
        return m_uniqueMap.value(id);
    }

    virtual inline SchemaNodePtr value(const SchemaQName &id) {
        return m_uniqueMap.value(id);
    }

    virtual inline const QMap<SchemaQName, SchemaNodePtr> & uniqueMap() const {
        return m_uniqueMap;
    }

    virtual inline int size() const {
        return m_uniqueMap.size();
    }
    
    virtual inline void append(const SchemaNodeMap &other) {
        foreach(SchemaNodePtr n, other.values()) {
            insert(n);
        }
    }

protected:
    QMap<SchemaQName, SchemaNodePtr> m_uniqueMap;
};


/** The base class of everything in a schema */
class KDEVSGMLDUCHAIN_EXPORT SchemaNode : public KShared  {

public:
    enum SchemaNodeType {
        None,
        SimpleType,
        ComplexType,
        Group,
        AttributeGroup,
        Element,
        Attribute,
        Enum,
        Schema
    };

    SchemaNode(const SchemaQName &qn) : m_qname(qn) {
    }

    virtual ~SchemaNode() {
    }

    virtual inline KDevelop::IndexedString name() const {
        return m_qname.name;
    }

    virtual inline KDevelop::IndexedString ns() const {
        return m_qname.ns;
    }

    virtual inline SchemaQName qname() const {
        return m_qname;
    }

    virtual inline void setQname(const SchemaQName &q) {
        m_qname = q;
    }

    virtual inline KDevelop::IndexedString annotation() const {
        return m_annotation;
    }

    virtual inline void setAnnotation(const KDevelop::IndexedString &a) {
        m_annotation = a;
    }

    virtual inline SchemaNodeType nodeType() const {
        return None;
    }

    virtual inline SchemaNodeMap elements() const {
        SchemaNodeMap ret;
        foreach (SchemaNodePtr base, m_bases) {
            ret.append(base->elements());
        }
        ret.append(m_elements);
        return ret;
    }

    virtual inline SchemaNodeMap attributes() const {
        SchemaNodeMap ret;
        foreach (SchemaNodePtr base, m_bases) {
            ret.append(base->attributes());
        }
        ret.append(m_attributes);
        return ret;
    }

    virtual inline SchemaNodeMap enums() const {
        SchemaNodeMap ret;
        foreach (SchemaNodePtr base, m_bases) {
            ret.append(base->enums());
        }
        ret.append(m_enums);
        return ret;
    }

    /** Adds or replaces a node as a child or base to this node based on the context of this node */
    virtual void add(SchemaNodePtr &n) = 0;

protected:
    SchemaQName m_qname;
    KDevelop::IndexedString m_annotation;
    /** Base type or unions */
    QList<SchemaNodePtr> m_bases;
    SchemaNodeMap m_elements;
    SchemaNodeMap m_attributes;
    SchemaNodeMap m_enums;
};


class KDEVSGMLDUCHAIN_EXPORT SchemaEnum : public SchemaNode {
public:
    SchemaEnum(const SchemaQName &qn) : SchemaNode(qn) {}

    virtual inline SchemaNodeType nodeType() const {
        return SchemaNode::Enum;
    }

    virtual inline void add(SchemaNodePtr &n) {
        Q_UNUSED(n);
    }

    virtual inline KDevelop::IndexedString value() const {
        return m_value;
    }

    virtual inline void setValue(const KDevelop::IndexedString &v) {
        m_value = v;
    }
protected:
    KDevelop::IndexedString m_value;
};


class KDEVSGMLDUCHAIN_EXPORT SchemaAttribute : public SchemaNode {
public:
    SchemaAttribute(const SchemaQName &qn) : SchemaNode(qn) {}

    virtual inline SchemaNodeType nodeType() const {
        return SchemaNode::Attribute;
    }

    virtual inline void add(SchemaNodePtr &n) {
        if (!n) return;
        if (n->nodeType() == Enum)
            m_enums.insert(n);
        else if (n->nodeType() == SimpleType)
            m_bases.append(n);
    }
};


class KDEVSGMLDUCHAIN_EXPORT SchemaElement : public SchemaNode {
public:
    SchemaElement(const SchemaQName &qn) : SchemaNode(qn) {}

    virtual inline SchemaNodeType nodeType() const {
        return SchemaNode::Element;
    }

    virtual inline void add(SchemaNodePtr &n) {
        if (!n) return;
        if (n->nodeType() == Element)
            m_elements.insert(n);
        else if (n->nodeType() == Group)
            m_elements.append(n->elements());
        else if (n->nodeType() == Attribute)
            m_attributes.insert(n);
        else if (n->nodeType() == AttributeGroup)
            m_attributes.append(n->attributes());
        else if (n->nodeType() == SimpleType)
            m_bases.append(n);
        else if (n->nodeType() == ComplexType)
            m_bases.append(n);
    }
};


class KDEVSGMLDUCHAIN_EXPORT SchemaComplexType : public SchemaNode {
public:
    SchemaComplexType(const SchemaQName &qn) : SchemaNode(qn) {}

    virtual inline SchemaNodeType nodeType() const {
        return SchemaNode::ComplexType;
    }

    virtual inline void add(SchemaNodePtr &n) {
        if (!n) return;
        if (n->nodeType() == Element)
            m_elements.insert(n);
        else if (n->nodeType() == Group)
            m_elements.append(n->elements());
        else if (n->nodeType() == Attribute)
            m_attributes.insert(n);
        else if (n->nodeType() == AttributeGroup)
            m_attributes.append(n->attributes());
        else if (n->nodeType() == SimpleType)
            m_bases.append(n);
        else if (n->nodeType() == ComplexType)
            m_bases.append(n);
    }
};


class KDEVSGMLDUCHAIN_EXPORT SchemaSimpleType : public SchemaNode {
public:
    SchemaSimpleType(const SchemaQName &qn) : SchemaNode(qn) {}

    virtual inline SchemaNodeType nodeType() const {
        return SchemaNode::SimpleType;
    }

    virtual inline void add(SchemaNodePtr &n) {
        if (!n) return;
        if (n->nodeType() == Attribute)
            m_attributes.insert(n);
        else if (n->nodeType() == AttributeGroup)
            m_attributes.append(n->attributes());
        else if (n->nodeType() == Enum)
            m_enums.insert(n);
        else if (n->nodeType() == SimpleType)
            m_bases.append(n);
    }
};


class KDEVSGMLDUCHAIN_EXPORT SchemaAttributeGroup : public SchemaNode {
public:
    SchemaAttributeGroup(const SchemaQName &qn) : SchemaNode(qn) {}

    virtual inline SchemaNodeType nodeType() const {
        return SchemaNode::AttributeGroup;
    }

    virtual inline void add(SchemaNodePtr &n) {
        if (!n) return;
        if (n->nodeType() == Attribute)
            m_attributes.insert(n);
        else if (n->nodeType() == AttributeGroup)
            m_attributes.append(n->attributes());
        else if (n->nodeType() == SimpleType)
            m_bases.append(n);
    }
};

class KDEVSGMLDUCHAIN_EXPORT SchemaGroup : public SchemaNode {
public:
    SchemaGroup(const SchemaQName &qn) : SchemaNode(qn) {}

    virtual SchemaNodeType nodeType() const {
        return SchemaNode::Group;
    }

    virtual inline void add(SchemaNodePtr &n) {
        if (!n) return;
        if (n->nodeType() == Element)
            m_elements.insert(n);
        else if (n->nodeType() == Group)
            m_elements.append(n->elements());
        else if (n->nodeType() == SimpleType)
            m_bases.append(n);
        else if (n->nodeType() == ComplexType)
            m_bases.append(n);
    }
};

typedef KSharedPtr<Schema> SchemaPtr;

/** The schema document */
class KDEVSGMLDUCHAIN_EXPORT Schema : public SchemaNode {
public:
    Schema(const KDevelop::IndexedString &ns, const KDevelop::IndexedString &doc) :
            SchemaNode(SchemaQName(ns,ns)),
            m_document(doc),
            m_qualifiedElements(false),
            m_qualifiedAttributes(false) {}

    virtual inline SchemaNodeType nodeType() const {
        return SchemaNode::Schema;
    }

    virtual inline KDevelop::IndexedString name() const {
        return SchemaNode::ns();
    }

    virtual inline KDevelop::IndexedString document() const {
        return m_document;
    }

    virtual inline void setDocument(const KDevelop::IndexedString &d) {
        m_document = d;
    }

    virtual inline bool qualifiedElements() const {
        return m_qualifiedElements;
    }

    virtual inline void setQualifiedElements(bool q) {
        m_qualifiedElements = q;
    }

    virtual inline bool qualifiedAttributes() const {
        return m_qualifiedAttributes;
    }

    virtual inline void setQualifiedAttributes(bool q) {
        m_qualifiedAttributes = q;
    }

    virtual inline void add(Xml::SchemaNodePtr& n) {
        if (!n) return;
        if (n->nodeType() == Element)
            m_elements.insert(n);
        else if (n->nodeType() == Group)
            m_groups.insert(n);
        else if (n->nodeType() == Attribute)
            m_attributes.insert(n);
        else if (n->nodeType() == AttributeGroup)
            m_attributeGroups.insert(n);
        else if (n->nodeType() == SimpleType)
            m_simpleTypes.insert(n);
        else if (n->nodeType() == ComplexType)
            m_complexTypes.insert(n);
        else if (n->nodeType() == Enum)
            m_enums.insert(n);
        //TODO include schema
    }
    
    /** includes a schema */
    virtual inline void add(SchemaPtr& s) {
        /*
        m_nodes.append(s->m_nodes);
        m_elements.append(s->elements());
        m_attributes.append(s->attributes());
        m_groups.append(s->groups());
        m_attributeGroups.append(s->attributeGroups());
        m_simpleTypes.append(s->simpleTypes());
        m_complexTypes.append(s->complexTypes());
        */
    }

    /** Adds a node into the global index, enums and attributes is not added */
    virtual inline void addNode(Xml::SchemaNodePtr& n) {
        if (!n) return;
        if (n->nodeType() == Enum || n->nodeType() == Attribute) return;
        m_nodes.insert(n);
    }

    virtual inline const SchemaNodePtr node(const SchemaQName &id) const {
        return m_nodes.value(id);
    }

    virtual inline SchemaNodePtr node(const SchemaQName &id) {
        return m_nodes.value(id);
    }

    virtual inline QList<SchemaNodePtr> nodes() const {
        return m_nodes.values();
    }

    virtual inline SchemaNodeMap elements() const {
        return m_elements;
    }

    virtual inline SchemaNodeMap attributes() const {
        return m_attributes;
    }

    virtual inline SchemaNodeMap enums() const {
        return m_enums;
    }

    virtual inline SchemaNodeMap groups() const {
        return m_groups;
    }

    virtual inline SchemaNodeMap attributeGroups() const {
        return m_attributeGroups;
    }

    virtual inline SchemaNodeMap complexTypes() const {
        return m_complexTypes;
    }

    virtual inline SchemaNodeMap simpleTypes() const {
        return m_simpleTypes;
    }

protected:
    /** The location of the document */
    KDevelop::IndexedString m_document;
    SchemaNodeMap m_nodes;
    SchemaNodeMap m_complexTypes;
    SchemaNodeMap m_simpleTypes;
    SchemaNodeMap m_attributeGroups;
    SchemaNodeMap m_groups;
    bool m_qualifiedElements;
    bool m_qualifiedAttributes;
    //QMap<KDevelop::IndexedString, KDevelop::IndexedString> m_imports;
};

typedef QMutex SchemaMutex;

class KDEVSGMLDUCHAIN_EXPORT SchemaController {
public:
    static SchemaController * self();

    inline QList<SchemaPtr> schemasForNamespace(const KDevelop::IndexedString &ns) {
        QList<SchemaPtr> list;
        foreach(SchemaPtr s, m_schemaNs.values()) {
            if(s->ns() == ns)
                list << s;
        }
        return list;
    }

    inline SchemaPtr schemaForDocument(const KUrl &doc) {
        return schemaForDocument(KDevelop::IndexedString(doc.pathOrUrl()));
    }

    inline SchemaPtr schemaForDocument(const KDevelop::IndexedString &doc) {
        if (m_schemaUrl.contains(doc))
            return m_schemaUrl.value(doc);
        return SchemaPtr();
    }

    inline SchemaNodePtr node(const SchemaQName &id) {
        foreach(SchemaPtr schema, schemasForNamespace(id.ns)) {
            if (schema) {
                SchemaNodePtr node = schema->node(id);
                if(node)
                    return node;
            }
        }
        return SchemaNodePtr();
    }

    /** Inserts or replaces a schema */
    inline void insertSchema(SchemaPtr &schema) {
        if (!schema || schema->nodeType() != SchemaNode::Schema)
            return;
        if(!m_schemaUrl.contains(schema->document())) {
            m_schemaNs.insert(schema->ns(), schema);
            m_schemaUrl.insert(schema->document(), schema);
        }
    }

    /** Mutex to lock the schema's
     *  A user may be working on a document while its schema's is being built.
     */
    inline SchemaMutex& mutex() {
        return m_mutex;
    }

protected:
    QMultiMap<KDevelop::IndexedString, SchemaPtr> m_schemaNs;
    QMap<KDevelop::IndexedString, SchemaPtr> m_schemaUrl;
private:
    static SchemaController * instance;
    mutable SchemaMutex m_mutex;
};

class SchemaMutexLocker : public QMutexLocker {
public:
    explicit SchemaMutexLocker() : QMutexLocker(& SchemaController::self()->mutex()) {}
};

void SchemaNodeMap::insert(SchemaNodePtr& node) {
    if (!node)
        return;
    m_uniqueMap.insert(node->qname(), node);
}

}

#endif // SCHEMA_H

