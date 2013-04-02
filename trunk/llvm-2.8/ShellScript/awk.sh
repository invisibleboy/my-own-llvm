awk 'BEGIN{count=0;sum=0}{sum+=$2; count++}END{print sum, count}' longWrite.txt >lwrite
