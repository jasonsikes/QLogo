#! /usr/bin/env python3

# Generate the header and table files for QLogo primitives from source files.
# by Jason Sikes

# To use:
# generate_command_table.py <source_dir> <dest_header_file> <dest_body_file>
#
# Or, more specifically:
# $ cd Projects/QLogo
# $ util/generate_command_table.py qlogo/executor include/primitives.h include/workspace/primitivetable.h

# Within the .cpp code in the qlogo/executor subdirectory are lines that start with "//CMD".
# One or more of these lines can precede the implementation method of a Logo primitive.
#
# The "//CMD" lines must immediately precede the implementation method. No gaps!
# The format is:
#
# //CMD <command-name> <arity-min> <arity-default> <arity-max>
#
# For example:
#
# //CMD PRINT 0 1 -1
# 
# means that the command is named "PRINT", the minimum number of inputs it will accept is zero,
# the default number of inputs is one, and the maximum number of inputs is "-1".
#
# Multiple "//CMD" lines may precede an implementation. In those cases multiple entries will be
# added to the primitives table.
#
# The arity list may contain special values:
#  min     = -1 -> The command and all inputs are read as a list without running,
#                  e.g. "TO PROC :P1" becomes [TO PROC :P1]. QLogo will not attempt
#                  to execute PROC nor attempt to lookup the variable :P1
#  default = -1 -> All parameters are consumed until end of line
#  max     = -1 -> All parameters are consumed within parens



import os, errno, sys, re

source_prefix = sys.argv[1]
dest_header = sys.argv[2]
dest_table = sys.argv[3]

print("opening '%s'" % (dest_header))
header_file = open(dest_header, 'w')
print("opening '%s'" % (dest_table))
table_file = open(dest_table, 'w')


SOURCES = [f for f in os.listdir(source_prefix) if re.match(r'.*\.cpp', f)]

# Return TRUE iff the given line is a command entry.

# A command entry is a line that starts with:
# "//CMD".
def is_command_entry(line):
    return line.lstrip().startswith("//CMD")

# Extract the elements of a command entry line
# The elements are: [NAME, min_params, default_params, max_params]
# The first result of the split is "//CMD" and is discarded.
def extract_command_elements(line):
    return line.strip().split()[1:]

# Find the next command entry line.
def find_next_command(file):
    while True:
        line = file.readline()
        if not line:
            return ''
        if is_command_entry(line):
            return line

# Return True if the given line is a Kernel method definition
def is_kernel_define(line):
    return line.lstrip().startswith("DatumPtr Kernel::")

# Return the name of the Kernel method given by line.
def method_name(line):
    # Remove the part before "::"
    sans_prefix = line.strip().split("::")[1]
    # Return the part before the "("
    return sans_prefix.split("(")[0]

# Write out the entry
def insert(entries, method):
    for entry in entries:
        [name, min_params, default_params, max_params] = entry
        table_file.write("stringToCmd[QObject::tr(\"%s\")] = {&Kernel::%s,%s,%s,%s};\n" % (name,method,min_params,default_params,max_params))
    header_file.write("DatumPtr %s(DatumPtr node);\n" % (method))


# Header and footer are for both generated files.
header = """
// DO NOT EDIT THIS FILE!!!

// It was generated by generate_command_table. Any edits to this file will
// not survive long.

#ifdef CONSTANTS_H

"""

footer = """

#endif // CONSTANTS_H

"""

table_file.write(header)
header_file.write(header)

    
for source in SOURCES:
    path = "%s/%s" % (source_prefix, source)
    print("Reading '%s'." % (path))
    file = open(path, 'r')
    entry_count = 0

    while True:
        line = find_next_command(file)
        if line == '':
            break
        entries = []
        while True:
            entries.append(extract_command_elements(line))
            line = file.readline()
            if not is_command_entry(line):
                break
        # Hopefully, the current line is the Kernel method definition.
        if is_kernel_define(line):
            method = method_name(line)
            insert(entries,method)
            entry_count += 1
        else:
            print("ERROR!\n")
            print("line: " + line)
            exit(1)
        
    print("Found %s entries" % (entry_count))

table_file.write(footer)
header_file.write(footer)
print("Finished!")
