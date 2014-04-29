H=localhost
P=15800
DATA='{"foo":"bar","blah":[1,2,3]}'

while [[ $1 == -* ]]; do
    case $1 in 
        -h) if (($# > 1)); then
                H=$1; 
                shift 2
            else
                echo '-h requires an arg' 1>&2 ;
                exit 1
            fi
        ;;
        -p) if (($# > 1)); then
                P=$2; 
                shift 2
            else 
                echo '-p requires an arg' 1>&2 ;
                exit 1
            fi
        ;;
        -d) if (($# > 1)); then 
                DATA=$2;
                shift 2
            else
                echo '-d requires an arg' 1>&2 ;
                exit 1
            fi
    esac
done
# retry with the correct master as many times as necessary
while true; do
    MASTER=$(./server_command -h $H -p $P -c "request" -d $DATA)
    echo $MASTER;
    echo $MASTER | grep -q "NACK";
    if [[ $? == 0 ]]; then 
        P=$(echo $MASTER | sed 's/.*"NACK","NOT_MASTER",".*",\([0-9]*\)\]/\1/');
        H=$(echo $MASTER | sed 's/.*"NACK","NOT_MASTER","\(.*\)",[0-9]*\]/\1/');
    else 
        break
    fi
done