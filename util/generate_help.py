#! /usr/bin/env python3

# Generate the SQLite help database from qlogo source files.
# by Jason Sikes

# To use:
# generate_help.py <cpp_source_dir> <library_helpfile.json> <dest_db>
#
# Where:
# 
# <cpp_source_dir> is the directory containing the C++ source code
# for the QLogo primitives.
# 
# <library_helpfile.json> is a JSON file that maps the library names to the
# help text for each library procedure.
# 
# <dest_db> is the path to the SQLite database to be created.
#
# Example:
# $ cd ~/Projects/QLogo
# $ util/generate_help.py qlogo/executor util/logolib_help.json share/qlogo_help.db

import os, errno, sqlite3, sys, re, json

source_prefix = sys.argv[1]
json_file_path = sys.argv[2]
dest_path = sys.argv[3]

SOURCES = [f for f in os.listdir(source_prefix) if re.match(r'.*\.cpp', f)]


# Find the next header line.
# A header line is the beginning of help text in the format:
# "/***DOC FORWARD FD" where the command has two names:
# "FORWARD" and "FD".
# Returns list of words from header (or empty string if eof).
def find_next_header(file):
    while True:
        line = file.readline()
        if not line:
            return ''
        if line.lstrip().startswith("/***DOC"):
            return line.strip().split()[1:]


# Read in the text of a documentation entry.
# Reads and collects text until end of documentation marker:
# "COD***/"
# Returns the entire help text as a string.
def read_text(file):
    retval = ''
    while True:
        line = file.readline()
        if line.lstrip().startswith("COD***/"):
            return retval
        retval = retval + line


# Insert an entry into the documentation database
def insert(conn,header,description):
    # The first value in the header list will be the key.
    command = header[0]
    # All of the values will be aliases.
    for alias in header:
        conn.execute('''INSERT INTO ALIASES (ALIAS,COMMAND) 
        VALUES (?,?)''', (alias,command))
    # Now insert the help text.
    conn.execute('''INSERT INTO HELPTEXT (COMMAND,DESCRIPTION) 
    VALUES (?,?)''', (command,description))
    conn.commit()
    


        
# Delete the database if it already exists
try:
    os.remove(dest_path)
    os.mkdir(os.path.dirname(dest_path))
except OSError:
    pass

conn = sqlite3.connect(dest_path)

# Some commands have more than one name and share a help text.
# We create at least one alias for every command name.
# An alias may be the same as the command. For example:
# The FORWARD command has an alias, FD. We include both
# FORWARD and FD in the ALIASES table which points to
# FORWARD, which is the key to the help text.

# In cases where a command has no aliases, the alias table
# entry points to the command itself.

conn.execute('''CREATE TABLE ALIASES
         (ALIAS TEXT PRIMARY KEY     NOT NULL,
         COMMAND          TEXT    NOT NULL);''')
conn.execute('''CREATE TABLE HELPTEXT
         (COMMAND TEXT PRIMARY KEY     NOT NULL,
         DESCRIPTION          TEXT)''')

# Read the C++ source code for the QLogo primitives and insert the help text
# into the SQLite database.
for source in SOURCES:
    path = "%s/%s" % (source_prefix, source)
    print("Reading '%s'." % (path))
    file = open(path, 'r')
    entries = 0

    while True:
        header = find_next_header(file)
        if header == '':
            break
        text = read_text(file)
        insert(conn,header,text)
        entries += 1

    print("Found %s entries" % (entries))


# The Logo standard library is not a part of the QLogo C++ source code.
# So the library and its help text are in separate JSON files.
# Read the JSON file and insert the library help text into the database.

print("Reading '%s'." % (json_file_path))
entries = 0
with open(json_file_path, 'r') as json_file:
    help_data = json.load(json_file)

for command, help_text_lines in help_data.items():
    help_text = '\n'.join(help_text_lines) + '\n'
    command_list = [command]
    # A special case: one of the commands has an alternate name. For that command,
    # include the alternate name in the ALIASES table.
    if command == 'FILEP':
        command_list.append('FILE?')
    insert(conn, command_list, help_text)
    entries += 1

print("Found %s entries" % (entries))

print("Finished!")
conn.close()
