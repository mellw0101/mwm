#!/bin/bash

# Initialize a flag to track the overall success of the script.
overall_success=true

check_and_install_apt() {
    PACKAGE=$1
    if dpkg-query -W -f='${Status}' $PACKAGE 2>/dev/null | grep -q "ok installed"; then
        echo "$PACKAGE is already installed."
        return 0
    else
        echo "$PACKAGE is not installed. Attempting to install..."
        if sudo apt-get install -y $PACKAGE; then
            echo "$PACKAGE installed successfully."
            return 1
        else
            echo "Failed to install $PACKAGE."
            return 2
        fi
    fi
}

check_and_install_pacman() {
    if pacman -Qi $PACKAGE >/dev/null 2>&1; then
        echo "$PACKAGE is already installed."
        return 0
    else
        echo "$PACKAGE is not installed. Attempting to install..."
        if sudo pacman -S --noconfirm $PACKAGE; then
            echo "$PACKAGE installed successfully."
            return 1
        else
            echo "Failed to install $PACKAGE."
            return 2
        fi
    fi
}

install_package_apt() {
    local PACKAGE=$1
    check_and_install_apt $PACKAGE
    local result=$?
    if [ $result -eq 2 ]; then
        overall_success=false
    fi
}

install_package_pacman() {
    local PACKAGE=$1
    check_and_install_pacman $PACKAGE
    local result=$?
    if [ $result -eq 2 ]; then
        overall_success=false
    fi
}

final_check() {
    if [ "$overall_success" = true ]; then
        echo "All packages are correctly installed or were successfully installed."
        exit 0
    else
        echo "Some packages failed to install. Check the error messages above."
        exit 1
    fi
}

install_dependencies() {
    # Check the distribution
    if [ -x "$(command -v apt)" ]; then
        # Debian/Ubuntu
        sudo apt update
        install_package_apt libpng++-dev
        install_package_apt libxcb1-dev 
        install_package_apt libxcb-xvmc0-dev
        install_package_apt libxcb-xvmc0-dev
        install_package_apt libxcb-xv0-dev
        install_package_apt libxcb-xtest0-dev
        install_package_apt libxcb-shm0-dev
        install_package_apt libxcb-randr0-dev
        install_package_apt libxcb-image0-dev
        install_package_apt libxcb-keysyms1-dev
        install_package_apt libxcb-icccm4-dev
        install_package_apt libxcb-sync0-dev
        install_package_apt libxcb-xinerama0-dev
        install_package_apt libxcb-xfixes0-dev
        install_package_apt libxcb-shape0-dev
        install_package_apt libxcb-present-dev
        install_package_apt libxcb-dri3-dev
        install_package_apt libxcb-dri2-0-dev
        install_package_apt libxcb-damage0-dev
        install_package_apt libxcb-composite0-dev
        install_package_apt libxcb-cursor-dev
        install_package_apt libxcb-xkb-dev
        install_package_apt libxkbcommon-dev
        install_package_apt libxkbcommon-x11-dev
        install_package_apt libxkbfile-dev
        install_package_apt libx11-xcb-dev
        install_package_apt libx11-dev
        install_package_apt libxext-dev
        install_package_apt libxfixes-dev
        install_package_apt libxdamage-dev
        install_package_apt libxrender-dev
        install_package_apt libxcomposite-dev
        install_package_apt libxinerama-dev
        install_package_apt libxrandr-dev
        install_package_apt libxss-dev
        install_package_apt libxtst-dev
        install_package_apt libxv-dev
        install_package_apt libxvmc-dev
        install_package_apt libxshmfence-dev
        install_package_apt libxxf86vm-dev
        install_package_apt clang
        install_package_apt build-essential
        install_package_apt libxcb-ewmh-dev
        install_package_apt libxcb-ewmh2
        install_package_apt libxcb-xrm-dev
        install_package_apt konsole
        install_package_apt xinit
        install_package_apt libimlib2-dev
        install_package_apt libxau-dev
        install_package_apt libpolkit-agent-1-dev
        install_package_apt wireless-tools
        install_package_apt libiw-dev
        install_package_apt libxcb-xinput-dev
        install_package_apt fonts-hack-ttf
        install_package_apt libc++-dev
        install_package_apt libc++abi-dev
        install_package_apt libpulse-dev
        install_package_apt libdbus-c++-dev
    elif [ -x "$(command -v pacman)" ]; then
        # Arch Linux
        sudo pacman -Syyu --noconfirm   
        install_package_pacman xcb-proto
        install_package_pacman libxcb
        install_package_pacman xcb-util
        install_package_pacman xcb-util-image
        install_package_pacman xcb-util-keysyms
        install_package_pacman xcb-util-renderutil
        install_package_pacman xcb-util-wm
        install_package_pacman xcb-util-cursor
        install_package_pacman libxkbcommon
        install_package_pacman libxkbcommon-x11
        install_package_pacman libxkbfile
        install_package_pacman libx11
        install_package_pacman libxext
        install_package_pacman libxfixes
        install_package_pacman libxdamage
        install_package_pacman libxrender
        install_package_pacman libxcomposite
        install_package_pacman libxinerama
        install_package_pacman libxrandr
        install_package_pacman libxss
        install_package_pacman libxtst
        install_package_pacman libxv
        install_package_pacman libxvmc
        install_package_pacman libxshmfence
        install_package_pacman libxxf86vm
        install_package_pacman libxi
        install_package_pacman xcb-util
        install_package_pacman xorg-fonts-misc
        install_package_pacman xcb-util-xrm
        install_package_pacman libpng
        install_package_pacman imlib2
        install_package_pacman wireless_tools
        install_package_pacman xorg-xinit
        install_package_pacman xf86-video-vesa
        install_package_pacman xorg-bdftopcf
        install_package_pacman xorg-docs
        install_package_pacman xorg-font-util
        install_package_pacman xorg-fonts-100dpi
        install_package_pacman xorg-fonts-75dpi
        install_package_pacman xorg-fonts-encodings
        install_package_pacman xorg-iceauth
        install_package_pacman xorg-mkfontscale
        install_package_pacman xorg-server
        install_package_pacman xorg-server-common
        install_package_pacman xorg-server-devel
        install_package_pacman xorg-server-xephyr
        install_package_pacman xorg-server-xnest
        install_package_pacman xorg-server-xvfb
        install_package_pacman xorg-sessreg
        install_package_pacman xorg-setxkbmap
        install_package_pacman xorg-smproxy
        install_package_pacman xorg-x11perf
        install_package_pacman xorg-xauth
        install_package_pacman xorg-xbacklight
        install_package_pacman xorg-xcmsdb
        install_package_pacman xorg-xcursorgen
        install_package_pacman xorg-xdpyinfo
        install_package_pacman xorg-xdriinfo
        install_package_pacman xorg-xev
        install_package_pacman xorg-xgamma
        install_package_pacman xorg-xhost
        install_package_pacman xorg-xinput
        install_package_pacman xorg-xkbcomp
        install_package_pacman xorg-xkbevd
        install_package_pacman xorg-xkbutils
        install_package_pacman xorg-xkill
        install_package_pacman xorg-xlsatoms
        install_package_pacman xorg-xlsclients
        install_package_pacman xorg-xmodmap
        install_package_pacman xorg-xpr
        install_package_pacman xorg-xprop
        install_package_pacman xorg-xrandr
        install_package_pacman xorg-xrdb
        install_package_pacman xorg-xrefresh
        install_package_pacman xorg-xset
        install_package_pacman xorg-xsetroot
        install_package_pacman xorg-xvinfo
        install_package_pacman xorg-xwayland
        install_package_pacman xorg-xwd
        install_package_pacman xorg-xwininfo
        install_package_pacman xorg-xwud
        install_package_pacman libc++
        install_package_pacman pulseaudio-alsa
        install_package_pacman libpulse
    else
        echo "Unsupported distribution. Please install dependencies manually."
        exit 1
    fi
}

# Main script starts here
install_dependencies
final_check