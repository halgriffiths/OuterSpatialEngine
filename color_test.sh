#!/bin/bash

cmd_str=""
while IFS= read -r line || [[ -n "$line" ]]; do
    cmd_str="${cmd_str};$line"
done < "$1"

# echo "Total command string:"
# echo $cmd_str

# black, blue, cyan, green, magenta, none, orange, purple, red, white, yellow
gnuplot -e "$cmd_str" | colout '@' green | colout '#' red | colout '%' blue | colout '&' magenta | colout '\$' yellow

exit 0
