#!/bin/sh
#
# DON'T EDIT THIS!
#
# CodeCrafters uses this file to test your code. Don't make any changes here!
#
# DON'T EDIT THIS!
set -e
tmpFile=$(mktemp)
# installed openssl through homebrew, so the compiler flags need to be different. 
# pkg-config finds the correct flags for openssl and curl for the current system and passes them to the compiler
gcc $(pkg-config --cflags --libs openssl) -lcurl app/*.c -o $tmpFile
exec "$tmpFile" "$@"