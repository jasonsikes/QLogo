#! /usr/bin/env python3

# Generate the SQLite help facility from qlogo source files.
# by Jason Sikes

# To use:
# generate_help.py <source_dir> <dest_db>
#
# $ cd Projects/QLogo
# $ scripts/generate_help.py src/qlogo/executor share/qlogo_help.db

import os, errno, sqlite3, sys, re

source_prefix = sys.argv[1]
dest_path = sys.argv[2]

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
# An alias may be the same as the command, e.g.:
# FORWARD and FD both point to FORWARD. FORWARD then is the
# key to the help text.
# MAKE is the only alias for MAKE. MAKE is the key to the
# help text.
conn.execute('''CREATE TABLE ALIASES
         (ALIAS TEXT PRIMARY KEY     NOT NULL,
         COMMAND          TEXT    NOT NULL);''')
conn.execute('''CREATE TABLE HELPTEXT
         (COMMAND TEXT PRIMARY KEY     NOT NULL,
         DESCRIPTION          TEXT)''')


for source in SOURCES:
    path = "%s/%s" % (source_prefix, source)
    print("Reading '%s'." % (path))
    file = open(path, 'r')

    while True:
        header = find_next_header(file)
        if header == '':
            break
        print("Found header: '%s'" % (' '.join(header)))
        text = read_text(file)
        insert(conn,header,text)
        

    print("closing '%s'" % (path))
print("closing connection")
conn.close()
