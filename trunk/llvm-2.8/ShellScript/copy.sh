for FILE in *.trace; do
	DIR=$(echo ${FILE}|sed -e 's/\.trace//')
	echo "mkdir $DIR"
	mkdir $DIR
	
	echo "mv $FILE to $DIR"
	mv $FILE $DIR
done 
