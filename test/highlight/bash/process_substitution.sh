diff <(sort -u a.txt) <(sort -u b.txt)
cat >(sed 's/x/y/g') <(printf '%s\n' foo)
