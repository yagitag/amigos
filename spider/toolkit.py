#!/usr/bin/env python3

import common
import parser

import glob
import sys
import os



class FilePath:
  def __init__(self, dir_path, file_name):
    self.dir_path = dir_path
    self.file_name = file_name
  def __repr__(self):
    return (self.dir_path, self.file_name)
  def __str__(self):
    return self.dir_path + '/' + self.file_name



class ToolKit:
  def __init__(self, config):
    self.config = config

  def restoreUidsDb(self):
    with open(self.config['UID_STORAGE']['bd_file'], 'wb') as out_fd:
      dirs = glob.glob(self.config['WORKER']['pages_dump'] + '*')
      tasks = []
      for d in dirs: tasks.extend(os.listdir(d))
      uids = [zlib.crc32(task.encode()) for task in tasks]
      for uid in sorted(uids):
        uid_bytes = uid.to_bytes(length = 4, byteorder = 'big')
        out_fd.write(uid_bytes)


  def findSubs(self, pattern, subs = None):
    result = set()
    if not subs:
      subs = []
      for subs_dir in glob.glob(self.config['WORKER']['subs_dump'] + '*'):
        sub_names = os.listdir(subs_dir)
        for sub_name in sub_names:
          subs.append(FilePath(subs_dir, sub_name))
    for sub in subs:
      try:
        with open(str(sub), 'r') as fd:
          if fd.read().find(pattern) != -1:
            result.add(sub)
      except Exception:
        print('There is bad file:', sub)
    return result


  def sortDocs(self, subs, pattern):
    subs_path = self.config['WORKER']['subs_dump']
    pages_path = self.config['WORKER']['pages_dump']
    #
    result = []
    for sub in subs:
      val = 0
      page = pages_path + str(sub)[len(subs_path):]
      with open(page, 'r') as fd:
        text = fd.read()
        if pattern.lower() in parser.getTitle(text).lower(): val |= 0b10
        if pattern.lower() in parser.getDescription(text).lower(): val |= 0b01
        result.append((val, sub.file_name))
    return list(sorted(result, key = lambda x: x[0], reverse = True))


  def getDocs(self, pattern):
    subs = self.findSubs(pattern)
    return self.sortDocs(subs, pattern)



if __name__ == '__main__':
  tasks = []
  if len(sys.argv) > 2:
    while len(sys.argv) != 2: tasks.append(sys.argv.pop())
  config = common.getConfig()
  tk = ToolKit(config)
  for task in tasks:
    print('request is "{0}"\nresults are:'.format(task))
    docs = tk.getDocs(task)
    for doc in docs:
      print(doc)
