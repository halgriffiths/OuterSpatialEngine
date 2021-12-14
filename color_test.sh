#!/bin/bash

cmd_str=""
while IFS= read -r line || [[ -n "$line" ]]; do
    cmd_str="${cmd_str};$line"
done < "$1"

echo "Total command string:"
echo $cmd_str

gnuplot -e "$cmd_str" | grep --color=always '*'

exit 0
