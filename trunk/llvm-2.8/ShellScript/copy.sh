#for FILE in *.trace; do
#	DIR=$(echo ${FILE}|sed -e 's/\.trace//')
#	echo "mkdir $DIR"
#	mkdir $DIR
	
#	echo "mv $FILE to $DIR"
#	mv $FILE $DIR
#done 

for FILE in *.statOptIlp; do
	NEWFILE=$(echo ${FILE}|sed -e 's/\.statOptIlp/_53000_4\.statOptIlp/')	
	
	echo "mv $FILE to $NEWFILE"
	mv $FILE $NEWFILE
done 
