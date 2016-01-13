#!/bin/bash

touch a
touch b
touch c

./simpsh --rdonly fakeFile | grep "Error: Failed to open file!" > /dev/null


#./simpsh --rdonly Makefile | grep "Error: Failed to open file!" > /dev/null
#should_fail "does not report file that exists"

#./simpsh --command 0 1 2 ls | grep "Error" > /dev/null
#should_succeed "using a non existent file descriptor should report the error"

rm a
rm b
rm c

if [ 1 -eq 0 ]; then

> "$tmp_file"
(./simpsh \
  --verbose \
  --wronly "$tmp_file" \
  --command 1 2 3 echo foo 2>&1 \
  --command 0 0 0 echo foo ) 2>/dev/null 1>/dev/null

grep "foo" "$tmp_file" > /dev/null
should_succeed "commands after failing commands should succeed"


./simpsh --verbose --command 2>&1 | grep "command requires an argument" > /dev/null
should_fail "empty command should have no options"


./simpsh --verbose --command 1 2 3 2>&1 | grep "command requires at least 4 options" > /dev/null
should_succeed "command requires at least 4 arguments"


./simpsh --verbose --command 1 2 3 2>&1 | grep "command requires at least 4 options" > /dev/null
should_succeed "command reports malformed command options to user on stderr"


./simpsh \
  --wronly /tmp/foo \
  --verbose \
  --command 0 0 0 echo foo \
  | grep "command 0 0 0 echo foo" > /dev/null
should_succeed "command tracks all command options"


./simpsh --wronly /tmp/foo --command 0 0 0 echo "foo";
grep foo /tmp/foo > /dev/null;
should_succeed "command can write to write only file"


> "$tmp_file"
echo_path=$(which echo);
./simpsh --wronly /tmp/foo --command 0 0 0 "$echo_path" "foo";
grep foo /tmp/foo > /dev/null;
should_succeed "path command can write to write only file"


> "$tmp_file"
big_string=$(cat misc/foo.txt);
./simpsh --wronly /tmp/foo --command 0 0 0 echo "$big_string";
grep oooooooooooooooooo /tmp/foo > /dev/null;
should_succeed "path command can write long strings to write only file"


> "$tmp_file"
./simpsh --rdonly /tmp/foo --command 0 0 0 echo "foo";
grep foo /tmp/foo > /dev/null;
# NOTE that failure of `echo "foo"` end up in stderr
should_fail "shouldn't be able to write to read only file"


echo "foo" > "$tmp_file"
cat "$tmp_file" | wc -l | grep 1 > /dev/null
should_succeed "the temporary file should have one line"
# the cat of $tmp_file should be empty and not add another line to tmp_file
./simpsh --wronly "$tmp_file" --command 0 0 0 cat "$tmp_file"
cat "$tmp_file" | wc -l | grep 1 > /dev/null
should_succeed "shouldn't be able to write to read only file"


echo "foo" > "$tmp_file"
echo "bar" > "$tmp_file2"

cat "$tmp_file" | grep "foo" > /dev/null
should_succeed "the temporary file should have 'foo'"

cat "$tmp_file2" | grep "bar" > /dev/null
should_succeed "the temporary file 2 should have 'bar'"

# cat of /tmp/foo should end up in the /tmp/file2
./simpsh --rdonly "$tmp_file" --wronly "$tmp_file2" --command 0 1 0 cat "$tmp_file"
cat "$tmp_file2" | grep "foo" > /dev/null && cat "$tmp_file2" | wc -l | grep 1 > /dev/null
should_succeed "should be able to cat from one file to the other (replace bar with foo)"


./simpsh --rdonly "$tmp_file" --command 0 0 0 echo "foo"
echo "$?" | grep "1" > /dev/null
should_succeed "exit status of failed subcommand should be the exit status of simpsh"


# NOTE: the bin/exit.sh provides an interesting exist value that should trump
# the first command's exit value of 1
./simpsh --rdonly "$tmp_file" --command 0 0 0 echo "foo" --command 0 0 0 bash bin/exit.sh
echo "$?" | grep "5" > /dev/null
should_succeed "max exit status of failed subcommand should be the exit status of simpsh"

./simpsh --rdonly misc/list.txt --wronly "$tmp_file" --command 0 1 0 sort -u
cat "$tmp_file" | xargs | grep "1 2 3 4 5" > /dev/null
should_succeed "short args for commands should be extracted propertly, sort -u should work"

./simpsh --verbose --rdonly misc/list.txt --wronly "$tmp_file" --command 0 1 0 sort -u > "$tmp_file"
grep "VERBOSE: --rdonly misc/list.txt" "$tmp_file" > /dev/null
should_succeed "should find verbose output of rdonly"
grep "VERBOSE: --wronly $tmp_file"  "$tmp_file" > /dev/null
should_succeed "should find verbose output of wronly"
grep "VERBOSE: --command 0 1 0 sort -u"  "$tmp_file" > /dev/null
should_succeed "should find verbose output of command"

./simpsh \
  --verbose \
  --rdonly misc/list.txt \
  --wronly "$tmp_file" \
  --command 0 1 0 sort -u \
  --wronly "$tmp_file" \
  > "$tmp_file"

grep "VERBOSE: --command 0 1 0 sort -u"  "$tmp_file" > /dev/null
should_succeed "should find verbose output of command"
tail -1 "$tmp_file" | grep "VERBOSE: --wronly $tmp_file"  "$tmp_file" > /dev/null
should_succeed "should find wronly after command (order of options)"

# TODO: test with larger number file descriptors

fi
