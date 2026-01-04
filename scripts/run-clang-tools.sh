#!/bin/bash
#
# Local script to run clang-format and clang-tidy on the codebase
# Usage: ./scripts/run-clang-tools.sh [format|tidy|both]
#

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Get the root directory of the repository
REPO_ROOT=$(git rev-parse --show-toplevel)
cd "$REPO_ROOT" || exit 1

# Default action
ACTION="${1:-both}"

# Check if tools are available
check_tool() {
    if ! command -v "$1" &> /dev/null; then
        echo -e "${RED}Error: $1 not found. Please install it.${NC}"
        exit 1
    fi
}

# Find C++ source files
find_cpp_files() {
    find qlogo Psychi include -type f \( -name "*.cpp" \) \
        ! -path "*/CMakeFiles/*" \
        ! -path "*/build/*" \
        ! -path "*_autogen/*"
}

# Run clang-format
run_format() {
    echo -e "${BLUE}Running clang-format...${NC}"
    check_tool clang-format
    
    local files
    files=$(find_cpp_files)
    
    if [ -z "$files" ]; then
        echo -e "${YELLOW}No C++ files found.${NC}"
        return
    fi
    
    local modified=0
    while IFS= read -r file; do
        if [ -f "$file" ]; then
            if ! clang-format --dry-run --Werror "$file" &> /dev/null; then
                echo -e "  Formatting: ${file}"
                clang-format -i "$file"
                modified=1
            fi
        fi
    done <<< "$files"
    
    if [ $modified -eq 1 ]; then
        echo -e "${GREEN}✓ Formatting complete.${NC}"
    else
        echo -e "${GREEN}✓ All files are already formatted.${NC}"
    fi
}

run_tidy() {
    echo -e "${BLUE}Running clang-tidy...${NC}"
    check_tool clang-tidy
    
    # Find the build directory with compile_commands.json
    # Check common build directory locations
    BUILD_DIR=""
    if [ -f "build/Desktop-Debug/compile_commands.json" ]; then
        BUILD_DIR="build/Desktop-Debug"
    elif [ -f "build/compile_commands.json" ]; then
        BUILD_DIR="build"
    elif [ -f "compile_commands.json" ]; then
        BUILD_DIR="."
    else
        # Try to find any build directory with compile_commands.json
        BUILD_DIR=$(find build -name "compile_commands.json" -type f 2>/dev/null | head -1 | xargs dirname 2>/dev/null || echo "")
    fi
    
    # If no compile_commands.json found, generate it
    if [ -z "$BUILD_DIR" ] || [ ! -f "$BUILD_DIR/compile_commands.json" ]; then
        echo -e "${YELLOW}Warning: compile_commands.json not found.${NC}"
        echo -e "${YELLOW}Generating it with CMake...${NC}"
        
        # Try to find existing build directory or create a default one
        if [ -d "build/Desktop-Debug" ]; then
            BUILD_DIR="build/Desktop-Debug"
            cd "$BUILD_DIR"
            cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON .
            cd "$REPO_ROOT"
        elif [ -d "build" ]; then
            BUILD_DIR="build"
            cd "$BUILD_DIR"
            cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON ..
            cd "$REPO_ROOT"
        else
            mkdir -p build
            BUILD_DIR="build"
            cd "$BUILD_DIR"
            cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON ..
            cd "$REPO_ROOT"
        fi
        
        # Verify it was created
        if [ ! -f "$BUILD_DIR/compile_commands.json" ]; then
            echo -e "${RED}Error: Failed to generate compile_commands.json${NC}"
            exit 1
        fi
    fi
    
    # Set the path argument for clang-tidy
    if [ "$BUILD_DIR" = "." ]; then
        TIDY_ARGS="-p ."
    else
        TIDY_ARGS="-p $BUILD_DIR"
        echo -e "${BLUE}Using compile_commands.json from: $BUILD_DIR${NC}"
    fi
    
    # Verify that MOC-generated headers exist (if Qt is used)
    if [ -d "$BUILD_DIR/qlogo/qlogo_autogen/include" ] || [ -d "$BUILD_DIR/Psychi/Psychi_autogen/include" ]; then
        echo -e "${GREEN}✓ Found MOC-generated headers in build directory${NC}"
    else
        echo -e "${YELLOW}Warning: MOC-generated headers not found. You may need to build the project first.${NC}"
        echo -e "${YELLOW}  Expected locations:${NC}"
        echo -e "${YELLOW}    - $BUILD_DIR/qlogo/qlogo_autogen/include${NC}"
        echo -e "${YELLOW}    - $BUILD_DIR/Psychi/Psychi_autogen/include${NC}"
    fi
    
    # Verify LLVM headers are accessible
    if [ -d "/usr/lib/llvm-19/include" ] || [ -d "/usr/include/llvm" ]; then
        echo -e "${GREEN}✓ LLVM headers found${NC}"
    else
        echo -e "${YELLOW}Warning: LLVM headers not found in expected locations.${NC}"
    fi
    
    local files
    files=$(find_cpp_files)
    
    if [ -z "$files" ]; then
        echo -e "${YELLOW}No C++ files found.${NC}"
        return
    fi
    
    local output_file="clang_tidy.out"
    local error_count=0
    
    echo -e "Writing results to ${output_file}..."
    
    > "$output_file"  # Clear the file
    
    while IFS= read -r file; do
        if [ -f "$file" ]; then
            echo -e "  Checking: ${file}"
            # Exclude system headers and generated files from analysis
            # Only analyze headers in project directories, explicitly exclude /usr paths
            if ! clang-tidy $TIDY_ARGS --header-filter='^(?!/usr/).*/(qlogo|Psychi|include)/.*' "$file" >> "$output_file" 2>&1; then
                error_count=$((error_count + 1))
            fi
        fi
    done <<< "$files"
    
    # Count warnings and errors
    local warning_count
    warning_count=$(grep -c "warning:" "$output_file" 2>/dev/null || echo "0")
    local error_count_in_file
    error_count_in_file=$(grep -c "error:" "$output_file" 2>/dev/null || echo "0")
    
    echo -e "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
    echo -e "${BLUE}clang-tidy Results:${NC}"
    echo -e "  Warnings: ${warning_count}"
    echo -e "  Errors: ${error_count_in_file}"
    echo -e "  Output file: ${output_file}"
    echo -e "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
    
    if [ "$warning_count" -eq 0 ] && [ "$error_count_in_file" -eq 0 ]; then
        echo -e "${GREEN}✓ No issues found!${NC}"
    else
        echo -e "${YELLOW}⚠ Issues found. See ${output_file} for details.${NC}"
        echo -e "${YELLOW}First 20 lines:${NC}"
        head -n 20 "$output_file"
    fi
}

# Main execution
case "$ACTION" in
    format)
        run_format
        ;;
    tidy)
        run_tidy
        ;;
    both)
        run_format
        echo ""
        run_tidy
        ;;
    *)
        echo -e "${RED}Error: Unknown action '$ACTION'${NC}"
        echo "Usage: $0 [format|tidy|both]"
        exit 1
        ;;
esac




