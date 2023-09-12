source "$(dirname "${BASH_SOURCE[0]}")/_env.sh"

# common variables used by multiple scripts
FILES=$(find \( -path './build*' -prune -false \) -or \( -path './extern*' -prune -false \) -or -name '*.cpp' -or -name '*.hpp')
