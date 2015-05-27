#!/usr/bin/env python3

import datetime
import os


class Log():
  def __init__(self, log_file):
    self.log_file = log_file
    self.log_fd = open(log_file, 'a')


  def __del__(self):
    self.log_fd.close()


  def write(self, info):
    print("{0}: {1}".format(datetime.datetime.now(), info), file = self.log_fd)


  def rotateLogs(self):
    self.log_fd.close()
    os.rename(self.log_file, self.log_file + '.0')
    self.rotateLog(self.log_file + '.0')
    self.log_fd = open(self.log_file, 'a')


  def rotateLog(self, log_file):
    cur_val = int(log_file[-1])
    new_log_file = log_file[:-1] + str(cur_val + 1)
    if os.path.exists(new_log_file):
      self.rotateLog(new_log_file)
    os.rename(log_file, new_log_file)


