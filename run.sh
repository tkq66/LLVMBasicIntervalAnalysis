if  [ $1 = "1" ]; then
    bin/IntervalPass resources/example1.ll x
elif [ $1 = "2" ]; then
    bin/IntervalPass resources/example2.ll x
elif [ $1 = "3" ]; then
    bin/IntervalPass resources/example3.ll y
else
    echo "Please enter the option 1, 2, or 3 for the example file you would like to run difference analysis on."
fi
