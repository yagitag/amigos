#!/usr/bin/env python3

import parser
import common

import time
import glob
import zlib
import sys
import os


class Converter:
  def __init__(self, config):
    self.results_path = config['CONVERTER']['results_path']
    self.flush_after = config['CONVERTER']['flush_after']
    self.common_pages_path = config['WORKER']['pages_dump']
    self.common_subs_path = config['WORKER']['subs_dump']
    if not os.path.exists(self.results_path):
      os.makedirs(self.results_path)
    self.openNewFile(True)

  def openNewFile(self, is_first_time = False):
    self.char_cnt = 0
    if not is_first_time:
      self.write('</items>')
      fd.close()
      raise Exception
    self.fd = open(self.results_path + '/' + str(int(time.time())), 'w')
    self.write('<?xml version="1.0" encoding="utf-8" ?>')
    self.write('<items>')

  def write(self, data):
    self.char_cnt += self.fd.write(data)

  def run(self):
    dirs_sfx = [pdir[-len('_??-??-????'):] for pdir in glob.glob(self.common_pages_path + '_??-??-????')]
    for sfx in dirs_sfx:
      page_dir = self.common_pages_path + sfx
      uids = os.listdir(page_dir)
      for uid in uids:
        info = {'video_id': uid, 'id': zlib.crc32(uid.encode())}
        with open(page_dir + '/' + uid, 'r') as fd:
          text = fd.read()
        info.update(self.parsePage(text))
        with open(self.common_subs_path + sfx + '/' + uid, 'r') as fd:
          text = fd.read()
        info.update(self.parseSub(text))
        self.save(info)

  def parsePage(self, text):
    result['title'] = parser.getTitle(text)
    result['description'] = parser.getDescription(text)
    result['keywords'] = parser.getKeywords(text)
    result['loudness'] = parser.getLoudness(text)
    result.update(parser.getMetaInfo(text))
    result.update(parser.getUserInfo(text))
    result.update(parser.getLikes(text))
    return result

  def writeDictAsXml(self, dictionary, shift = 0):
    str_shift = ' ' * shift
    for (tag, content) in dictionary.items():
      if isinstance(content, dict):
        self.write('{0}<{1}>'.format(str_shift, tag))
        self.writeDictAsXml(content, fd, shift + 2)
        self.write('{0}</{1}>'.format(str_shift, tag))
      else:
        self.write(fd.write('{0}<{1}>{2}</{1}>'.format(str_shift, tag, content)))

  def parseSub(self, text):
    return parser.parseSub(text)

  def save(self, info):
    self.write('  <item>')
    self.writeDictAsXml(info, 4)
    self.write('  </item>')
    if self.char_cnt > self.flush_after:
      self.openNewFile()




if __name__ == '__main__':
  config = common.getConfig()
  c = Converter(config)
  c.run()

