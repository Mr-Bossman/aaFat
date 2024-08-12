#!/bin/bash
random_string() {
	cat /dev/urandom | tr -dc 'a-z0-9' | fold -w 15 | head -n 1
}

random_int() {
	echo $(($2 + RANDOM % $1))
}

get_md5() {
	md5sum -b $1 | awk '{print $1}'
}

rm -f test/*
for i in {1..100}; do
	FILENAME=$(random_string)
	SIZE=$(random_int 10 10)
	BS=$(random_int 1024 1)
	dd if=/dev/random of=/tmp/$FILENAME bs=$BS count=$SIZE 2>/dev/null
	HASH_B=$(get_md5 /tmp/$FILENAME)
	dd if=/tmp/$FILENAME of=test/$FILENAME bs=$BS count=$SIZE 2>/dev/null
	HASH_A=$(get_md5 test/$FILENAME)
	if [ "$HASH_A" == "$HASH_B" ]; then
		printf "\r$i"
		rm /tmp/$FILENAME
		rm test/$FILENAME
	else
		echo ""
		echo $HASH_A $HASH_B
		echo test/$FILENAME
		echo "SIZE: $((SIZE * BS))"
		echo "FAIL"
		exit 1
	fi
done
echo ""
