#! /usr/bin/env python3

# Generate the SQLite QLogo standard library from the given JSON file.
# by Jason Sikes

# To use:
# generate_library.py
#
# Can be run from any directory. Paths are resolved relative to the
# repository root (the parent of this script's directory):
#
#   json_file_path: util/logolib.json
#   dest_path:      share/qlogo_library.db

import os, sqlite3, json

repo_root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
json_file_path = os.path.join(repo_root, 'util', 'logolib.json')
dest_path = os.path.join(repo_root, 'share', 'qlogo_library.db')

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
