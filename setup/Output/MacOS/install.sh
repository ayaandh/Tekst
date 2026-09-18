#!/usr/bin/env bash
set -e

echo "Installing Tekst 2.1.3..."

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"

case "$OSTYPE" in
    darwin*)
        OS="macOS"
        CONFIG_DIR="$HOME/Library/Application Support/tekst"
        ;;
    linux*)
        OS="Linux"
        CONFIG_DIR="${XDG_CONFIG_HOME:-$HOME/.config}/tekst"
        ;;
    *)
        echo "Error: This installer only supports Linux and macOS."
        exit 1
        ;;
esac

echo "Detected: $OS"

INSTALL_DIR="$HOME/.tekst"
BIN_DIR="$INSTALL_DIR/bin"
LIB_DIR="$INSTALL_DIR/lib"
PACKAGES_DIR="$INSTALL_DIR/packages"

mkdir -p \
    "$BIN_DIR" \
    "$LIB_DIR" \
    "$PACKAGES_DIR" \
    "$CONFIG_DIR" \
    "$CONFIG_DIR/packages"

REQUIRED_FILES=(
    "$SCRIPT_DIR/bin/tekst"
    "$SCRIPT_DIR/bin/tk"
    "$SCRIPT_DIR/runtime.cpp"
    "$SCRIPT_DIR/runtime.h"
)

for FILE in "${REQUIRED_FILES[@]}"; do
    if [ ! -f "$FILE" ]; then
        echo "Error: Required file not found:"
        echo "  $FILE"
        exit 1
    fi
done

cp "$SCRIPT_DIR/bin/tekst" "$BIN_DIR/"
cp "$SCRIPT_DIR/bin/tk" "$BIN_DIR/"
cp "$SCRIPT_DIR/runtime.cpp" "$LIB_DIR/"
cp "$SCRIPT_DIR/runtime.h" "$LIB_DIR/"

chmod +x "$BIN_DIR/tekst"
chmod +x "$BIN_DIR/tk"

if [ "$OS" = "Linux" ] && command -v update-mime-database >/dev/null 2>&1; then
    mkdir -p "$HOME/.local/share/mime/packages"

    cat > "$HOME/.local/share/mime/packages/tekst.xml" <<EOF
<?xml version="1.0" encoding="UTF-8"?>
<mime-info xmlns="http://freedesktop.org/standards/shared-mime-info">
    <mime-type type="text/x-tekst">
        <comment>Tekst Source File</comment>
        <glob pattern="*.tk"/>
        <glob pattern="*.tekst"/>
    </mime-type>
</mime-info>
EOF

    update-mime-database "$HOME/.local/share/mime" || true
fi

case "$(basename "$SHELL")" in
    zsh)
        SHELL_PROFILE="$HOME/.zshrc"
        ;;
    bash)
        if [ -f "$HOME/.bashrc" ]; then
            SHELL_PROFILE="$HOME/.bashrc"
        else
            SHELL_PROFILE="$HOME/.profile"
        fi
        ;;
    *)
        SHELL_PROFILE="$HOME/.profile"
        ;;
esac

PATH_LINE="export PATH=\"\$PATH:$BIN_DIR\""

if [ -f "$SHELL_PROFILE" ] && grep -Fq "$BIN_DIR" "$SHELL_PROFILE"; then
    echo "Tekst is already in PATH."
else
    printf '\n# Tekst Compiler Path\n%s\n' "$PATH_LINE" >> "$SHELL_PROFILE"
    echo "Added Tekst to PATH in $SHELL_PROFILE"
fi

export PATH="$PATH:$BIN_DIR"

echo ""
echo "Tekst 2.1.3 installed successfully!"
echo "Installation directory: $INSTALL_DIR"
echo "Run 'tekst' or 'tk' after restarting your terminal."