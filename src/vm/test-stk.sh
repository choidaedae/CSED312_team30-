cnt=0

file_path="test.txt"
search_string="PANIC"

for (( i=0; i<100; i=i+1 ))
  do
    make clean && make
    cd build
    echo "$cnt / 100"
    pintos -v -k -T 120 --qemu  --filesys-size=2 -p tests/vm/page-merge-stk -a page-merge-stk -p tests/vm/child-qsort -a child-qsort --swap-size=4 -- -q  -f run page-merge-stk > test.txt

    output=$( tail -n 9 test.txt )
    output=($output)

    IFS=' ' read -r -a array <<< "${output[0]}"
    failed_cnt=${array[3]}

    if [ $failed_cnt = "complete." ]
    then
        cnt=$((cnt+1))
        cd ../
    else
        echo "Fail!!!"
        cd ../
        tar -cvf ../../build_$cnt.tar build
        break
    fi
    
done
