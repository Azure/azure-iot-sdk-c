#!/bin/bash

repo_name_from_uri()
{
    echo "$1" | sed -e 's|/$||' -e 's|:*/*\.git$||' -e 's|.*[/:]||g'
}

scriptdir=$(cd "$(dirname "$0")" && pwd)
deps="openssl git cmake pkgconfig ossp-uuid valgrind"
repo="https://github.com/Azure/azure-iot-sdk-c.git"
repo_name=$(repo_name_from_uri $repo)

push_dir () { pushd $1 > /dev/null; }
pop_dir () { popd $1 > /dev/null; }

repo_exists ()
{
    push_dir "$scriptdir"
    [ "$(git rev-parse --is-inside-work-tree)" == "true" ] || return 1
    origin=$(git config remote.origin.url) || return 1
    actual_name=$(repo_name_from_uri $origin)

    # Perform a caseless compare of the repo name
    shopt -s nocasematch
    [ "${repo_name}" == "${actual_name}" ] || return 1
    pop_dir
}

brew_prereq()
{
    #
    # Check if Homebrew is installed
    #
    which -s brew
    if [[ $? != 0 ]] ; then
        # Install Homebrew
        echo "Homebrew is required to proceed. Installing Homebrew..."
        /usr/bin/ruby -e "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/master/install)"
    else
        echo "Updating Homebrew..."
        brew update
    fi
}

deps_install ()
{
    brew_prereq
    echo "-------------------------------------------------"
    echo "Ensuring required dependencies...."    
    brew install $deps
    echo "---------------------------------------------Done"
}

clone_source ()
{
    git clone --recursive $repo
}

deps_install

if repo_exists
then
    echo "Repo $repo_name already cloned"
    push_dir "$(git rev-parse --show-toplevel)"
else
    clone_source || exit 1
    push_dir "$repo_name"
fi

pop_dir