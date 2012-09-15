for DIR in *; do 
	if [ "$DIR" == "." -o "$DIR" == ".." ]; then
		echo $DIR
		continue
	fi

	cd $DIR
	echo "==Current in: $(pwd)"
	
	LockStat=$(find -type f|ls *.lockStat)	

	for FILE in $LockStat; do 
		
		NewFILE=$FILE"25"
		echo "mv $FILE $NewFILE "
		mv $FILE $NewFILE 	

		if [ $? -ne 0 ]; then
			echo "Error renaming"
			exit 
		fi	
	done
	cd ..
	echo "==Back to $(pwd)"

done
	
