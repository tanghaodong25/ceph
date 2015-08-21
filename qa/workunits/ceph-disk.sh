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
exit 0
source $(dirname $0)/ceph-helpers.sh
source $(dirname $0)/ceph-helpers-root.sh

TIMEOUT=360
TEST_POOL=rbd

function test_pool_read_write() {
    local id=$1

    timeout $TIMEOUT ceph osd pool set $TEST_POOL size 1 || return 1

    local weight=1
    ceph osd crush add osd.$id $weight root=default host=localhost || return 1
    echo FOO > $DIR/BAR
    timeout $TIMEOUT rados --pool $TEST_POOL put BAR $DIR/BAR || return 1
    timeout $TIMEOUT rados --pool $TEST_POOL get BAR $DIR/BAR.copy || return 1
    diff $DIR/BAR $DIR/BAR.copy || return 1
}

function test_activate() {
    local to_prepare=$1
    local journal=$2
    local osd_uuid=$($uuidgen)

    mkdir -p $OSD_DATA

    ceph-disk $CEPH_DISK_ARGS \
        prepare --osd-uuid $osd_uuid $to_prepare $journal || return 1

    local to_activate=$(ceph-disk list --format json $to_prepare | jq '

    timeout $TIMEOUT ceph-disk $CEPH_DISK_ARGS \
        activate \
        $to_activate || return 1

    local id=$(ceph osd create $osd_uuid)

    test_pool_read_write $id || return 1

    control_osd stop $id
}

function test_activate_dmcrypt() {
    local to_prepare=$1
    local to_activate=$2
    local journal=$3
    local journal_p=$4
    local uuid=$5
    local juuid=$6
    local plain=$7

    mkdir -p $OSD_DATA

    if test $plain = plain ; then
        echo "osd_dmcrypt_type=plain" > $DIR/ceph.conf
    fi
    
    ceph-disk $CEPH_DISK_ARGS \
		prepare --dmcrypt --dmcrypt-key-dir $DIR/keys --osd-uuid=$uuid --journal-uuid=$juuid $to_prepare $journal || return 1

    if test $plain = plain ; then
        /sbin/cryptsetup --key-file $DIR/keys/$uuid --key-size 256 create $uuid $to_activate
        /sbin/cryptsetup --key-file $DIR/keys/$juuid --key-size 256 create $juuid $journal
    else
        /sbin/cryptsetup --key-file $DIR/keys/$uuid.luks.key luksOpen $to_activate $uuid
        /sbin/cryptsetup --key-file $DIR/keys/$juuid.luks.key luksOpen ${journal}${journal_p} $juuid
    fi

    timeout $TIMEOUT ceph-disk $CEPH_DISK_ARGS \
        activate \
        --mark-init=none \
        /dev/mapper/$uuid || return 1

    test_pool_read_write $uuid || return 1
}

function activate_dev_body() {
    local disk=$1
    local journal=$2
    local newdisk=$3

    #
    # Create an OSD without a journal and an objectstore
    # that does not use a journal.
    #
    ceph-disk zap $disk || return 1
    CEPH_ARGS="$CEPH_ARGS --osd-objectstore=memstore" \
        test_activate $disk || return 1

    #
    # Create an OSD with data on a disk, journal on another
    #
    ceph-disk zap $disk || return 1
    test_activate $disk ${disk}p1 $journal || return 1
    kill_daemons
    umount ${disk}p1 || return 1
    teardown

    setup
    run_mon
    #
    # Create an OSD with data on a disk, journal on another
    # This will add a new partition to $journal, the previous
    # one will remain.
    #
    ceph-disk zap $disk || return 1
    test_activate $disk ${disk}p1 $journal || return 1
    kill_daemons
    umount ${disk}p1 || return 1
    teardown

    setup
    run_mon
    #
    # Create an OSD and reuse an existing journal partition
    #
    test_activate $newdisk ${newdisk}p1 ${journal}p1 || return 1
    #
    # Create an OSD and get a journal partition from a disk that
    # already contains a journal partition which is in use. Updates of
    # the kernel partition table may behave differently when a
    # partition is in use. See http://tracker.ceph.com/issues/7334 for
    # more information.
    #
    ceph-disk zap $disk || return 1
    test_activate $disk ${disk}p1 $journal || return 1
    kill_daemons
    umount ${newdisk}p1 || return 1
    umount ${disk}p1 || return 1
    teardown
}

function TEST_activate_dev() {
    test_setup_dev_and_run activate_dev_body
}

function test_setup_dev_and_run() {
    local action=$1
    if test $(id -u) != 0 ; then
        echo "SKIP because not root"
        return 0
    fi

    local disk=/dev/vdb
    ceph-disk zap $disk
    local journal=/dev/vdc
    ceph-disk zap $journal
    local newdisk=/dev/vdd
    ceph-disk zap $newdisk

    $action $disk $journal $newdisk
}

function activate_dmcrypt_dev_body() {
    local disk=$1
    local journal=$2
    local newdisk=$3
    local uuid=$($uuidgen)
    local juuid=$($uuidgen)

    setup
    run_mon
    test_activate_dmcrypt $disk ${disk}p1 $journal p1 $uuid $juuid not_plain || return 1
    kill_daemons
    umount /dev/mapper/$uuid || return 1
    teardown
}

function TEST_activate_dmcrypt_dev() {
    test_setup_dev_and_run activate_dmcrypt_dev_body
}

function activate_dmcrypt_plain_dev_body() {
    local disk=$1
    local journal=$2
    local newdisk=$3
    local uuid=$($uuidgen)
    local juuid=$($uuidgen)

    setup
    run_mon
    test_activate_dmcrypt $disk ${disk}p1 $journal p1 $uuid $juuid plain || return 1
    kill_daemons
    umount /dev/mapper/$uuid || return 1
    teardown
}

function TEST_activate_dmcrypt_plain_dev() {
    test_setup_dev_and_run activate_dmcrypt_plain_dev_body
}

function run() {
    local funcs=${@:-$(set | sed -n -e 's/^\(TEST_[0-9a-z_]*\) .*/\1/p')}
    for func in $funcs ; do
        setup $dir || return 1
        $func $dir || return 1
        teardown $dir || return 1
    done
}

main ceph-disk "$@"
