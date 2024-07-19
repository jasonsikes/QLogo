#! /usr/bin/env python3

# Generate the SQLite QLogo standard library from the given JSON file.
# by Jason Sikes

# To use:
# generate_library.py <library_source.json> <dest_db>
#
# Where:
# 
# <library_source.json> is a JSON file that contains the code for the
# QLogo standard library.
# 
# <dest_db> is the path to the SQLite database to be created.
#
# Example:
# $ cd ~/Projects/QLogo
# $ util/generate_library.py util/logolib.json share/qlogo_library.db

import os, sqlite3, sys, json

json_file_path = sys.argv[1]
dest_path = sys.argv[2]

# Delete the database if it already exists
try:
    os.remove(dest_path)
    os.mkdir(os.path.dirname(dest_path))
except OSError:
    pass

conn = sqlite3.connect(dest_path)

# Create the table for the QLogo standard library
conn.execute('''CREATE TABLE LIBRARY
         (COMMAND TEXT PRIMARY KEY NOT NULL,
         CODE TEXT NOT NULL);''')

# Read the JSON file and insert the library code into the database
print("Reading '%s'." % (json_file_path))
entries = 0
with open(json_file_path, 'r') as json_file:
    library_data = json.load(json_file)

for command, code_lines in library_data.items():
    code = '\n'.join(code_lines) + '\n'
    conn.execute('''INSERT INTO LIBRARY (COMMAND, CODE) 
    VALUES (?,?)''', (command, code))
    entries += 1

print("Found %s entries" % (entries))
conn.commit()
print("Finished!")
conn.close()
