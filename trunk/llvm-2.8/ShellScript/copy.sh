#for FILE in *.trace; do
#	DIR=$(echo ${FILE}|sed -e 's/\.trace//')
#	echo "mkdir $DIR"
#	mkdir $DIR
	
#	echo "mv $FILE to $DIR"
#	mv $FILE $DIR
#done 

for FILE in *.statOpt; do
	NEWFILE=$(echo ${FILE}|sed -e 's/\.statOpt/_53000_4\.statOpt/')	
	
	echo "mv $FILE to $NEWFILE"
	mv $FILE $NEWFILE
done 

for FILE in *.statBase; do
	NEWFILE=$(echo ${FILE}|sed -e 's/\.statBase/_53000_4\.statBase/')	
	
	echo "mv $FILE to $NEWFILE"
	mv $FILE $NEWFILE
done 
