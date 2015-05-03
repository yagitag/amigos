#!/usr/bin/env python3

import configparser
import sys
import os

def getConfig(config_path = None):
  if not config_path:
    assert len(sys.argv) == 2, 'Usage: {0} path_to_config'.format(sys.argv[0])
    config_path = sys.argv[1]
  assert os.path.exists(config_path), "No such file or directory: '{0}'".format(config_path)
  config = configparser.ConfigParser(interpolation = configparser.ExtendedInterpolation(), inline_comment_prefixes = ('#'))
  config.read(config_path)
  return config

if __name__ == '__main__':
  config = getConfig()

