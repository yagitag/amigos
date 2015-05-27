#!/usr/bin/env python3

import os

class UIdsStorage():
  def __init__(self, uid_size, bd_file, max_uids_on_ram_cnt):
    self.proc_uids = set()
    self.ram_uids = set()
    self.max_cnt = int(max_uids_on_ram_cnt)
    self.bd_file = bd_file
    self.uid_size = int(uid_size)
    if not os.path.exists(self.bd_file):
      open(self.bd_file, 'w').close()
    self.bd_fd = open(self.bd_file, 'rb')

  def __del__(self):
    self.bd_fd.close()

  def __len__(self):
    hdd_size = os.stat(self.bd_file).st_size // self.uid_size
    return len(self.ram_uids) + hdd_size

  def __contains__(self, uid):
    exist = False
    if uid in self.proc_uids: exist = True
    elif uid in self.ram_uids: exist = True
    elif self.isAtHdd(uid): exist = True
    return exist

  def getHddValue(self, offset, from_what = os.SEEK_CUR):
    self.bd_fd.seek(offset, from_what)
    return self.bd_fd.read(self.uid_size)

  def isAtHdd(self, uid):
    val = uid.to_bytes(length = self.uid_size, byteorder = 'big')
    bd_size = os.stat(self.bd_file).st_size // self.uid_size
    (l_pos, r_pos) = (0, bd_size)
    while l_pos < r_pos:
      m_pos = (l_pos + r_pos) // 2
      m_val = self.getHddValue(m_pos * self.uid_size, os.SEEK_SET)
      if m_val < val:
        l_pos = m_pos + 1
      elif m_val > val:
        r_pos = m_pos
      else: return True
    return False

  def add(self, new_uid):
    result = False
    if new_uid not in self:
      self.proc_uids.add(new_uid)
      result = True
    return result

  def commit(self, uid):
    self.proc_uids.remove(uid)
    self.ram_uids.add(uid)
    if len(self.ram_uids) > self.max_cnt:
      self.flushToDisk()

  def flushToDisk(self):
    if not self.ram_uids: return
    tmp_bd_name = self.bd_file + '.tmp'
    self.bd_fd.seek(0)
    with open(tmp_bd_name, 'wb') as out_fd:
      ram_uids = iter(sorted(self.ram_uids))
      hdd_uid_bytes = self.bd_fd.read(self.uid_size)
      try:
        ram_uid_bytes = next(ram_uids).to_bytes(length = self.uid_size, byteorder = 'big')
        while hdd_uid_bytes:
          if ram_uid_bytes <= hdd_uid_bytes:
            out_fd.write(ram_uid_bytes)
            ram_uid_bytes = next(ram_uids).to_bytes(length = self.uid_size, byteorder = 'big')
          else:
            out_fd.write(hdd_uid_bytes)
            hdd_uid_bytes = self.bd_fd.read(self.uid_size)
        out_fd.write(ram_uid_bytes)
        for ram_uid in ram_uids:
          ram_uid_bytes = ram_uid.to_bytes(length = self.uid_size, byteorder = 'big')
          out_fd.write(ram_uid_bytes)
      except StopIteration:
        out_fd.write(hdd_uid_bytes)
        out_fd.write(self.bd_fd.read())
    self.bd_fd.close()
    self.ram_uids.clear()
    os.rename(tmp_bd_name, self.bd_file)
    self.bd_fd = open(self.bd_file, 'rb')

if __name__ == '__main__':
  result = [True for _ in range(4)] + [False, False, False]
  uids = UIdsStorage(4, 'uids.db', 100)
  print(len(uids))
  
  for uid in (1, 4, 5, 7):
    uids.add(uid)
  tmp_result = [uid in uids for uid in (1, 4, 5, 7, 3, 9, 0)]
  if tmp_result != result: print(tmp_result)
  print(len(uids))
  #
  for uid in (1, 7):
    try: uids.commit(uid)
    except KeyError: pass
  tmp_result = [uid in uids for uid in (1, 4, 5, 7, 3, 9, 0)]
  if tmp_result != result: print(tmp_result)
  print(len(uids))
  #
  uids.flushToDisk()
  tmp_result = [uid in uids for uid in (1, 4, 5, 7, 3, 9, 0)]
  if tmp_result != result: print(tmp_result)
  print(len(uids))
  #
  #uids.commit(4)
  tmp_result = [uid in uids for uid in (1, 4, 5, 7, 3, 9, 0)]
  if tmp_result != result: print(tmp_result)
  print(len(uids))
