H="localhost"
P=15800
DELAY=1
while [[ $1 == -* ]]; do
    case $1 in 
        -h) if (($# > 1)); then
                H=$2; 
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
                D=$2;
                shift 2
            else
                echo '-d requries and arg' 1>&2 ;
                exit 1
            fi
    esac
done
while true; do 
    MASTER=$(./do_get_master.sh -h $H -p $P);
    echo $MASTER | grep -q "NACK";
    if [[ $? == 0 ]]; then 
        P=$(echo $MASTER | sed 's/.*"NACK","NOT_MASTER",".*",\([0-9]*\)\]/\1/');
        H=$(echo $MASTER | sed 's/.*"NACK","NOT_MASTER","\(.*\)",[0-9]*\]/\1/');
    else 
        break;
    fi
done
./server_command -h $H -p $P -c "stop";
sleep $DELAY;
./server_command -h $H -p $P -c "start"
