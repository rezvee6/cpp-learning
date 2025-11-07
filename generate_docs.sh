#!/bin/bash

# Script to generate Doxygen documentation

set -e

echo "Generating Doxygen documentation..."

# Check if Doxygen is installed
if ! command -v doxygen &> /dev/null; then
    echo "Error: Doxygen is not installed."
    echo ""
    echo "Installation instructions:"
    echo "  macOS:    brew install doxygen graphviz"
    echo "  Ubuntu:   sudo apt-get install doxygen graphviz"
    echo "  Windows:  Download from https://www.doxygen.nl/download.html"
    exit 1
fi

# Check if Graphviz is installed (for diagrams)
if ! command -v dot &> /dev/null; then
    echo "Warning: Graphviz (dot) is not installed. Class diagrams will not be generated."
    echo "Install with: brew install graphviz (macOS) or sudo apt-get install graphviz (Ubuntu)"
fi

# Run Doxygen
doxygen Doxyfile

if [ $? -eq 0 ]; then
    echo ""
    echo "Documentation generated successfully!"
    echo "Open docs/html/index.html in your browser to view the documentation."
else
    echo "Error: Documentation generation failed."
    exit 1
fi

