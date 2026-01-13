#!/bin/bash
#
# Install git hooks from util/git-hooks/ to .git/hooks/
# This script should be run after cloning the repository
#

set -e

# Colors
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

# Get the root directory of the repository
REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$REPO_ROOT"

HOOKS_SOURCE_DIR="$REPO_ROOT/util/git-hooks"
HOOKS_TARGET_DIR="$REPO_ROOT/.git/hooks"

if [ ! -d "$HOOKS_SOURCE_DIR" ]; then
    echo "Error: Hooks directory not found: $HOOKS_SOURCE_DIR"
    exit 1
fi

if [ ! -d "$HOOKS_TARGET_DIR" ]; then
    echo "Error: Git hooks directory not found: $HOOKS_TARGET_DIR"
    echo "Are you in a git repository?"
    exit 1
fi

echo -e "${GREEN}Installing git hooks...${NC}"

# Install each hook
for hook in "$HOOKS_SOURCE_DIR"/*; do
    if [ -f "$hook" ]; then
        hook_name=$(basename "$hook")
        target_hook="$HOOKS_TARGET_DIR/$hook_name"
        
        # Copy the hook
        cp "$hook" "$target_hook"
        chmod +x "$target_hook"
        
        echo -e "  ✓ Installed: $hook_name"
    fi
done

echo -e "${GREEN}✓ Git hooks installed successfully!${NC}"
echo -e "${YELLOW}Note: Hooks will run automatically on git commit.${NC}"



