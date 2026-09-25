#! /usr/bin/env python3

# Generate util/logolib.json from the UCBLogo logolib directory.
# by Jason Sikes

# To use:
# generate_logolib_json.py
#
# Can be run from any directory. Paths are resolved relative to the
# repository root (the parent of this script's directory):
#
#   source_dir:     /usr/share/ucblogo/logolib
#   json_file_path: util/logolib.json
#
# UCBLogo stores each library procedure (or a small related group) as a
# file. Some filesystems cannot hold '?' in a name, so UCBLogo writes '?'
# as 'Q' (fileQ for FILE?, Qrest for ?REST). Keys in the JSON are the
# procedure names in Key form (uppercase).
#
# The Messages* files in that directory are error catalogs, not procedures,
# and are skipped.

import os, json

# UCBLogo logolib directory hardcoded for Fedora Linux.
source_dir = '/usr/share/ucblogo/logolib'

repo_root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
json_file_path = os.path.join(repo_root, 'util', 'logolib.json')


def filename_to_command(filename):
    return filename.replace('Q', '?').upper()


def is_message_catalog(filename):
    return filename.startswith('Messages')


def procedure_lines(path):
    with open(path, 'r', encoding='utf-8') as source:
        return [line.strip() for line in source.read().splitlines()]


def read_logolib(logolib_dir):
    library_data = {}
    skipped = 0
    for filename in sorted(os.listdir(logolib_dir)):
        path = os.path.join(logolib_dir, filename)
        if not os.path.isfile(path):
            continue
        if is_message_catalog(filename):
            print("Skipping message catalog '%s'." % (filename))
            skipped += 1
            continue
        command = filename_to_command(filename)
        library_data[command] = procedure_lines(path)
    return library_data, skipped


def main():
    if not os.path.isdir(source_dir):
        raise SystemExit("UCBLogo logolib directory not found: %s" % (source_dir))

    # Begin by erasing the current library JSON so leftover keys cannot survive.
    print("Erasing '%s'." % (json_file_path))
    with open(json_file_path, 'w', encoding='utf-8'):
        pass

    print("Reading '%s'." % (source_dir))
    library_data, skipped = read_logolib(source_dir)

    with open(json_file_path, 'w', encoding='utf-8') as json_file:
        json.dump(library_data, json_file, indent=4)
        json_file.write('\n')

    print("Wrote %s entries (%s skipped) to '%s'."
          % (len(library_data), skipped, json_file_path))
    print("Finished!")


if __name__ == '__main__':
    main()
