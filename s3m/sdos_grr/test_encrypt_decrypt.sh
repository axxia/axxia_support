#!/bin/bash

#exit when a command fails
set -e

trap 'last_command=$current_command; current_command=$BASH_COMMAND' DEBUG
trap 'echo "\"${last_command}\" command : Exit code $?."' EXIT




declare -a  DataSize=("1" "3" "4" "35" "128" "129" "186" "256" "257" "1024" "4096" "16384")


#delete results from previous tests
for val in "${DataSize[@]}" 
do
	rm -f ./test/test_encrypted_$val.bin
	rm -f ./test/test_decrypted_$val.bin
done






for val in "${DataSize[@]}" 
do
	echo "###############################################"
	echo "Encrypt/Decrypt With Size $val bytes"
	echo "###############################################"
	echo
	
	./sdos -i ./test/test_input_$val.bin -o ./test/test_encrypted_$val.bin -v 55555555555555555555555555555555  -d aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa -m encrypt
	./sdos -i ./test/test_encrypted_$val.bin -o ./test/test_decrypted_$val.bin  -d aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa -m decrypt
	
	file1="./test/test_input_$val.bin"
	file2="./test/test_decrypted_$val.bin"
	
	
	if cmp -s "$file1"  "$file2"; then
		printf 'File "%s" matches "%s", test sucessful \n', "$file1", "$file2"
	else
		printf 'File "%s" does not match "%s", test failed! \n', "$file1", "$file2"
		exit -1
	fi
done	







