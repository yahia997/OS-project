#!/bin/bash

make

echo "Running grouped test cases..."

current=""

# Read file after removing \r
while IFS= read -r line || [[ -n "$line" ]]
do
  if [[ "$line" == "---" ]]; then
    # Run current group
    if [[ -n "$current" ]]; then
      echo -e "\033[32mRunning shell...\033[0m"
      echo -e "\033[34m$current\033[0m"

      printf "%s\nexit\n" "$current" | ./myShell

      echo "-------------------"
      current=""
    fi
  else
    current+="$line"$'\n'
  fi
done < <(sed 's/\r$//' testcases.txt)

# Run last group (if no trailing ---)
if [[ -n "$current" ]]; then
  echo -e "\033[32mRunning shell...\033[0m"
  echo -e "\033[34m$current\033[0m"

  printf "%s\nexit\n" "$current" | ./myShell

  echo "-------------------"
fi

# remove any produced .txt files generated
find . -type f -name "*.txt" ! -name "testcases.txt" -delete