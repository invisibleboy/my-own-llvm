
if [ $# -lt 2 ]; then
	echo "lack of arguments"
	exit 
fi



DiffRec()
{

	for file in $1/*
	do
		cd $2
		#echo "current in $2"

		newfile=$(echo $file|sed 's,.*/,,')
		if [ ! -e $newfile ]; then
			echo "!!!no $2/$newfile"
		fi
		
		if [ -d $newfile ]; then
			DiffRec $file $2/$newfile
		fi		
	done
}


DiffRec $1 $2


