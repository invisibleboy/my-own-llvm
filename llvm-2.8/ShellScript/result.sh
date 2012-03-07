if [ $# -lt 1 ]; then
	echo "lack of args"
	exit 1
fi

if [ $# -lt 2 ]; then
	find . -name "*.sim" -exec grep -n $1 ./ {} \;|grep -v "\-2"| grep -v "\-4"|grep -v "\-6"|grep -v "\-8"|grep -v "\-0">../../log 
else
	find . -name "*0.sim" -exec grep -n $1 ./ {} \; >../../log 
fi


sed -i 's/-/0/g' ../../log 
more ../../log|sort> ../../log1
gedit ../../log1&

