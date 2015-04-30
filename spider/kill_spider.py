#!/usr/bin/env python3

import configparser
import signal
import sys
import os


if __name__ == '__main__':
  if len(sys.argv) == 2:
    config_file = sys.argv[1]
    if os.path.exists(config_file):
      config = configparser.ConfigParser(interpolation = configparser.ExtendedInterpolation())
      config.read(config_file)
      with open(config['SPIDER']['pid_file'], 'r') as fd:
        pid = int(fd.read())
      os.kill(pid, signal.SIGTERM)
    else:
      print('File "{0}" does not exist'.format(config_file))
  else:
    print('Usage: {0} path_to_config'.format(sys.argv[0]))
