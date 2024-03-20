#!/bin/bash

# Function to set feature flags
set_feature_flag()
{
    feature_name=$1
    while true; do
        echo "Enable $feature_name? [y/n]:"
        read reply
        case $reply in
            [Yy])
                export "${feature_name}_ENABLED=1";
                break
                ;;
            [Nn])
                export "${feature_name}_ENABLED=0";
                break
                ;;
            *)
                echo "Invalid input, please enter 'y' for yes or 'n' for no."
                ;;
        esac
    done
}

# Main script starts here.

set_feature_flag "NET_LOG"