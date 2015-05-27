#!/usr/bin/env python3

import common

import signal
import os


if __name__ == '__main__':
  config = common.getConfig()
  with open(config['SPIDER']['pid_file'], 'r') as fd:
    pid = int(fd.read())
  os.kill(pid, signal.SIGTERM)
