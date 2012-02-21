#!/bin/bash

rsync --exclude='glog/**' --exclude='gtest/**' --exclude='pin/**' --exclude='bin/**' --exclude='obj/**' -e ssh --numeric-ids --quiet --ignore-errors --archive --delete-during --force $CONCURRIT_HOME/ potus.cs.berkeley.edu:/home/elmas/concurrit/
