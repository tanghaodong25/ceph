#!/bin/bash
#
# Copyright (C) 2015 Red Hat <contact@redhat.com>
#
# Author: Loic Dachary <loic@dachary.org>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU Library Public License as published by
# the Free Software Foundation; either version 2, or (at your option)
# any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Library Public License for more details.
#
#######################################################################

##
#
# @param action either start or stop
# @param id osd identifier
# @return 0 on success, 1 on error
#

function control_osd() {
    local action=$1
    local id=$2
    
    local init=$(ceph-detect-init)

    case $init in 
        upstart)
            sudo service ceph-osd $action id=$id
            ;;
        *)
            echo ceph-detect-init returned an unknown init system: $init >&2
            return 1
            ;;
    esac
    return 0
}

function test_control_osd() {
    control_osd start 13245 || return 1
    control_osd stop 13245 || return 1
}

#######################################################################

function run_tests() {
    shopt -s -o xtrace
    PS4='${BASH_SOURCE[0]}:$LINENO: ${FUNCNAME[0]}:  '

    local funcs=${@:-$(set | sed -n -e 's/^\(test_[0-9a-z_]*\) .*/\1/p')}
    local dir=testdir/ceph-helpers-root

    for func in $funcs ; do
        $func $dir || return 1
    done
}

if test "$1" = TESTS ; then
    shift
    run_tests "$@"
fi
