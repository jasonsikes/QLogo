#ifndef DATUM_ASTNODE_H
#define DATUM_ASTNODE_H

#include "datum_datump.h"
#include <QVector>


/// \brief A node of QLogo's Abstract Syntax Tree.
///
/// Before execution, a list is parsed into a list of executable nodes. Each node
/// contains a name, a pointer to the KernelMethod that does the actual execution,
/// and an array of zero or more children.
class ASTNode : public Datum {
protected:
    QVector<DatumP> children;

public:

    /// A human-readable string. Usually the command name.
    DatumP nodeName;

    // TODO: This is badly misnamed! Should be called "method".
    // (This got caught in the mass renaming.)
    /// A pointer to the kernel method that should be called when executing this node.
    KernelMethod kernel;

    /// Add a child to the node.
    void addChild(DatumP aChild);

    /// Returns the child at the specified index.
    DatumP childAtIndex(unsigned index);

    /// Returns the number of children that this node owns.
    int countOfChildren();

    ASTNode(DatumP aNodeName);
    ASTNode(const char *aNodeName);
    ~ASTNode();
    DatumType isa();

    /// For debugging. To be used when printing out the AST.
    QString name();

    /// For debugging. To be used when printing out the AST.
    QString printValue(bool fullPrintp = false, int printDepthLimit = -1,
                       int printWidthLimit = -1);

    /// For debugging. To be used when printing out the AST.
    QString showValue(bool fullPrintp = false, int printDepthLimit = -1,
                      int printWidthLimit = -1);

    /// Not used. Returns nothing.
    DatumP first(void);
};


#endif // DATUM_ASTNODE_H
