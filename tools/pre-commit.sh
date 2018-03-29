#!/bin/sh

if git diff --cached --name-only | grep mbed_app.json; then
    echo "Rejecting commit because mbed_app.json is modified."
    echo "Remove mbed_app.json from index with 'git reset mbed_app.json'"
    echo "OR use 'git commit -s --no-verify' to skip this check"
    exit 1
fi
