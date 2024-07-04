#!/bin/bash

# creating test files
touch input_file.txt
touch output_file.txt

# filling the input file with 4096 a's
for ((i = 0; i < 4096; i++)); do
    echo -n a >>input_file.txt
done

# filling the input file with 4096 c's
for ((i = 0; i < 4096; i++)); do
    echo -n c >>input_file.txt
done

######## first test ########
echo running overflow test...
# compiling the file 'buffered_open.c and the test1.c'
gcc -o run buffered_open.c test1.c
chmod +x run
# running the test with a timeout of 3 seconds
timeout 3s ./run
# checking the exit status of the test
if [ $? -eq 0 ]; then
    # we check if there is a diff between the input file and the output file
    if diff input_file.txt output_file.txt >/dev/null; then
        echo -e "\e[32mTEST WITH PREAPPEND-OFF : PASSED\e[0m"
    else
        echo -e "\e[31mTEST WITH PREAPPEND-OFF : FAILED\e[0m"
    fi
else
    echo -e "\e[31mTEST WITH PREAPPEND-OFF : FAILED - TIME PASSED\e[0m"
fi

# remove the run file and output_file
rm output_file.txt
rm run

echo

######## second test ########
echo running overflow PREAPPEND test...
# we recreate the output_file
touch output_file.txt

# adding 'BBB' to the begging of the file
for ((i = 0; i < 3; i++)); do
    echo -n B >>output_file.txt
done

# compiling the file 'buffered_open.c and the test2.c'
gcc -o run buffered_open.c test2.c
chmod +x run
# running the test with a timeout of 5 seconds
timeout 3s ./run
# checking the exit status of the test
if [ $? -eq 0 ]; then
    # we expect to see with the PREAPPEND ON "aaa..cc..BBB"
    # we add those 'BBB' also to the end of input_file

    for ((i = 0; i < 3; i++)); do
        echo -n B >>input_file.txt
    done

    # we will check if there is a diff between the input file and the output file
    if diff input_file.txt output_file.txt >/dev/null; then
        echo -e "\e[32mTEST WITH PREAPPEND-ON : PASSED\e[0m"
    else
        echo -e "\e[31mTEST WITH PREAPPEND-ON : FAILED\e[0m"
    fi
else
    echo -e "\e[31mTEST WITH PREAPPEND-ON : FAILED - TIME PASSED\e[0m"
fi

# remove all files
rm output_file.txt
rm run
echo

######## third test ########
touch output_file.txt
echo running cheating tests...
# compiling the file 'buffered_open.c and the test3.c'
gcc -o run buffered_open.c test3.c
chmod +x run
# running the test
./run
rm run
rm output_file.txt

######## fourth test ########
touch output_file.txt
echo running cheating PREAPPEND tests...
# compiling the file 'buffered_open.c and the test4.c'
gcc -o run buffered_open.c test4.c
chmod +x run
# running the test
./run

rm run
rm output_file.txt

######## roee's test ########
echo running roee\'s tests...
# compiling the file 'buffered_open.c and the roee.c'
gcc -o run buffered_open.c roee.c
chmod +x run
# running the test with a timeout of 5 seconds
./run
echo

rm run
rm Test3Output.txt
rm input_file.txt

# End of script
