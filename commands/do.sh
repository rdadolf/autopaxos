H=localhost
P=15800
C=start

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
        -c) if (($# > 1)); then 
                C=$2;
                shift 2
            else
                echo '-c requires an arg' 1>&2 ;
                exit 1
            fi
    esac
done
./server_command -h $H -p $P -c $C