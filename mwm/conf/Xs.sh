#!/bin/bash
# Start X server on display :1
X :1 vt2 &
sleep 2  # Wait for X server to start

# Set the DISPLAY environment variable
export DISPLAY=:1

# Start your window manager here, replace 'your_window_manager' with the actual command
exec ../test
